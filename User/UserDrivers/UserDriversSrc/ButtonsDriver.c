#include "FreeRTOS.h"
#include "task.h"

#include "stm32f1xx.h"

#include "ButtonsDriver.h"
#include "UserTimer.h"

//! Состояния пинов кнопок (кнопки подтянуты к 3.3 В, нажатие = низкий уровень)
#define BUTTONS_DRIVER_BUTTON_1_STATE   ((uint8_t)((GPIOA->IDR & GPIO_IDR_IDR10_Msk) >> GPIO_IDR_IDR10_Pos))
#define BUTTONS_DRIVER_BUTTON_2_STATE   ((uint8_t)((GPIOA->IDR & GPIO_IDR_IDR11_Msk) >> GPIO_IDR_IDR11_Pos))
#define BUTTONS_DRIVER_BUTTON_3_STATE   ((uint8_t)((GPIOA->IDR & GPIO_IDR_IDR12_Msk) >> GPIO_IDR_IDR12_Pos))
#define BUTTONS_DRIVER_BUTTON_4_STATE   ((uint8_t)((GPIOA->IDR & GPIO_IDR_IDR15_Msk) >> GPIO_IDR_IDR15_Pos))
#define BUTTONS_DRIVER_BUTTON_5_STATE   ((uint8_t)((GPIOB->IDR & GPIO_IDR_IDR12_Msk) >> GPIO_IDR_IDR12_Pos))
#define BUTTONS_DRIVER_BUTTON_6_STATE   ((uint8_t)((GPIOB->IDR & GPIO_IDR_IDR13_Msk) >> GPIO_IDR_IDR13_Pos))
#define BUTTONS_DRIVER_BUTTON_7_STATE   ((uint8_t)((GPIOB->IDR & GPIO_IDR_IDR14_Msk) >> GPIO_IDR_IDR14_Pos))
#define BUTTONS_DRIVER_BUTTON_8_STATE   ((uint8_t)((GPIOB->IDR & GPIO_IDR_IDR15_Msk) >> GPIO_IDR_IDR15_Pos))

//! Маска одного бита в упакованном состоянии кнопок
#define BUTTONS_DRIVER_RAW_STATE_BIT_MASK    0x01u

//! Начальное состояние упакованного поля (все пины отпущены)
#define BUTTONS_DRIVER_RAW_STATE_INIT        0xFFu

//! Начальная маска разрешённых событий.
//! По умолчанию включено только короткое нажатие — это дает
//! минимальную задержку отклика. Приложение само включает
//! DOUBLE, LONG, REPEAT и HOLD_TRACK в тех экранах, где они нужны.
#define BUTTONS_DRIVER_ENABLED_EVENTS_INIT   (BUTTONS_DRIVER_ENABLE_SHORT)

//! Логическое состояние пина кнопки
typedef enum
{
    BUTTON_ON = 0,   //!< Кнопка нажата (низкий уровень на пине)
    BUTTON_OFF       //!< Кнопка отпущена (высокий уровень на пине)
} ButtonState;

//! Контекст одной кнопки
typedef struct
{
    ButtonsDriverState state;                    //!< Текущее состояние конечного автомата
    uint16_t           debounceCounter;          //!< Счётчик антидребезга
    uint32_t           holdStartTimeMs;          //!< Время начала удержания кнопки в миллисекундах
    uint32_t           secondClickTicks;         //!< Счётчик ожидания второго нажатия
    uint32_t           repeatTicks;              //!< Счётчик автоповтора при удержании
    uint32_t           lastHoldDurationMs;       //!< Последнее время удержания в миллисекундах
    uint32_t           lastPressTimeMs;          //!< Момент последнего нажатия (после антидребезга)
    uint32_t           lastReleaseTimeMs;        //!< Момент последнего отпускания (после антидребезга)
} ButtonsDriverContext;

//! Массив структур с контекстами кнопок
static ButtonsDriverContext buttonsDriverContexts[BUTTONS_DRIVER_BUTTON_COUNT] = { 0 };

//! Упакованные события всех кнопок (по BUTTONS_DRIVER_EVENT_BITS бит на кнопку)
static volatile uint32_t buttonsEvents = 0u;

//! Маска разрешённых событий
static uint8_t enabledEvents = BUTTONS_DRIVER_ENABLED_EVENTS_INIT;

//! Указатель на callback, вызываемый из прерывания при появлении события
static void (*volatile pButtonsDriverEventCallback)(void) = NULL;

//! \brief Прочитать упакованное состояние всех кнопок в один байт
//! \return Упакованное состояние кнопок (0 = нажато, 1 = отпущено)
static uint8_t ButtonsDriver_ReadRawState(void)
{
    uint8_t rawState = 0u;

    rawState |= (BUTTONS_DRIVER_BUTTON_1_STATE << BUTTON_1);
    rawState |= (BUTTONS_DRIVER_BUTTON_2_STATE << BUTTON_2);
    rawState |= (BUTTONS_DRIVER_BUTTON_3_STATE << BUTTON_3);
    rawState |= (BUTTONS_DRIVER_BUTTON_4_STATE << BUTTON_4);
    rawState |= (BUTTONS_DRIVER_BUTTON_5_STATE << BUTTON_5);
    rawState |= (BUTTONS_DRIVER_BUTTON_6_STATE << BUTTON_6);
    rawState |= (BUTTONS_DRIVER_BUTTON_7_STATE << BUTTON_7);
    rawState |= (BUTTONS_DRIVER_BUTTON_8_STATE << BUTTON_8);

    return rawState;
}

//! \brief Проверить, разрешено ли событие
//! \param[in] event Событие кнопки
//! \return 1, если событие разрешено, иначе 0
static uint8_t ButtonsDriver_IsEventEnabled(ButtonsDriverEvent event)
{
    if (BUTTONS_DRIVER_EVENT_SHORT == event)
    {
        // Короткое нажатие разрешено всегда
        return 1u;
    }

    if (BUTTONS_DRIVER_EVENT_DOUBLE == event)
    {
        return ((enabledEvents & BUTTONS_DRIVER_ENABLE_DOUBLE) != 0u) ? 1u : 0u;
    }

    if (BUTTONS_DRIVER_EVENT_LONG == event)
    {
        return ((enabledEvents & BUTTONS_DRIVER_ENABLE_LONG) != 0u) ? 1u : 0u;
    }

    if (BUTTONS_DRIVER_EVENT_REPEAT == event)
    {
        return ((enabledEvents & BUTTONS_DRIVER_ENABLE_REPEAT) != 0u) ? 1u : 0u;
    }

    if (BUTTONS_DRIVER_EVENT_HOLD_START == event)
    {
        return ((enabledEvents & BUTTONS_DRIVER_ENABLE_HOLD_TRACK) != 0u) ? 1u : 0u;
    }

    if (BUTTONS_DRIVER_EVENT_HOLD_END == event)
    {
        return ((enabledEvents & BUTTONS_DRIVER_ENABLE_HOLD_TRACK) != 0u) ? 1u : 0u;
    }

    return 0u;
}

//! \brief Проверить, считается ли состояние FSM «нажатой» кнопкой
//! \param[in] state Состояние конечного автомата
//! \return 1, если состояние соответствует нажатой кнопке, иначе 0
static uint8_t ButtonsDriver_IsStatePressed(ButtonsDriverState state)
{
    if ((BUTTONS_DRIVER_STATE_PRESSED == state) ||
        (BUTTONS_DRIVER_STATE_LONG_FIRED == state))
    {
        return 1u;
    }

    return 0u;
}

//! \brief Установить ожидающее событие кнопки и уведомить задачу через callback
//! \details Функция вызывается ТОЛЬКО из ISR (через ProcessStateMachine).
//!          Критическая секция здесь не используется по двум причинам:
//!          1. ISR не может прервать сам себя — запись в buttonsEvents
//!             атомарна относительно текущего прерывания.
//!          2. Других писателей в buttonsEvents в текущей архитектуре нет —
//!             только TIM2 через ProcessTick.
//!          Вызов taskENTER_CRITICAL из ISR недопустим и приведёт к configASSERT.
//! \param[in] buttonId Идентификатор кнопки
//! \param[in] event Событие кнопки
static void ButtonsDriver_SetPendingEvent(Buttons buttonId, ButtonsDriverEvent event)
{
    if (0u == ButtonsDriver_IsEventEnabled(event))
    {
        return;
    }

    uint8_t eventShift = BUTTONS_DRIVER_EVENT_SHIFT(buttonId);

    buttonsEvents &= (uint32_t)~(BUTTONS_DRIVER_EVENT_MASK << eventShift);
    buttonsEvents |= (uint32_t)((uint32_t)event << eventShift);

    if (pButtonsDriverEventCallback != NULL)
    {
        pButtonsDriverEventCallback();
    }
}

//! \brief Обработчик одного тика для одной кнопки
//! \param[in] buttonId Идентификатор кнопки
//! \param[in,out] pContext Указатель на контекст кнопки
//! \param[in] rawState Сырое состояние кнопки на текущем тике
//! \param[in] currentTimeMs Текущее время работы программы в миллисекундах
static void ButtonsDriver_ProcessStateMachine(Buttons buttonId, ButtonsDriverContext *pContext,
                                              ButtonState rawState, uint32_t currentTimeMs)
{
    switch (pContext->state)
    {
        case BUTTONS_DRIVER_STATE_RELEASED:
        {
            if (BUTTON_ON == rawState)
            {
                pContext->debounceCounter = 0;
                pContext->state = BUTTONS_DRIVER_STATE_DEBOUNCE_DOWN;
            }

            break;
        }

        case BUTTONS_DRIVER_STATE_DEBOUNCE_DOWN:
        {
            if (BUTTON_ON == rawState)
            {
                pContext->debounceCounter++;

                if (pContext->debounceCounter >= BUTTONS_DRIVER_DEBOUNCE_TICKS)
                {
                    // Начало удержания подтверждено — запоминаем время старта
                    pContext->holdStartTimeMs = currentTimeMs;
                    pContext->lastPressTimeMs = currentTimeMs;
                    pContext->state = BUTTONS_DRIVER_STATE_PRESSED;

                    ButtonsDriver_SetPendingEvent(buttonId, BUTTONS_DRIVER_EVENT_HOLD_START);
                }
            }
            else
            {
                pContext->state = BUTTONS_DRIVER_STATE_RELEASED;
            }

            break;
        }

        case BUTTONS_DRIVER_STATE_PRESSED:
        {
            uint32_t holdDurationMs = currentTimeMs - pContext->holdStartTimeMs;

            if ((0u != ButtonsDriver_IsEventEnabled(BUTTONS_DRIVER_EVENT_LONG)) &&
                (holdDurationMs >= BUTTONS_DRIVER_LONG_PRESS_MS))
            {
                ButtonsDriver_SetPendingEvent(buttonId, BUTTONS_DRIVER_EVENT_LONG);

                pContext->repeatTicks = 0;
                pContext->state = BUTTONS_DRIVER_STATE_LONG_FIRED;
            }
            else if (BUTTON_OFF == rawState)
            {
                pContext->debounceCounter = 0;
                pContext->state = BUTTONS_DRIVER_STATE_DEBOUNCE_UP;
            }

            break;
        }

        case BUTTONS_DRIVER_STATE_DEBOUNCE_UP:
        {
            if (BUTTON_OFF == rawState)
            {
                pContext->debounceCounter++;

                if (pContext->debounceCounter >= BUTTONS_DRIVER_DEBOUNCE_TICKS)
                {
                    // Конец удержания подтверждён — сохраняем длительность и момент отпускания
                    pContext->lastHoldDurationMs = currentTimeMs - pContext->holdStartTimeMs;
                    pContext->lastReleaseTimeMs = currentTimeMs;
                    ButtonsDriver_SetPendingEvent(buttonId, BUTTONS_DRIVER_EVENT_HOLD_END);

                    if (0u != ButtonsDriver_IsEventEnabled(BUTTONS_DRIVER_EVENT_DOUBLE))
                    {
                        // Ждём возможного второго нажатия
                        pContext->secondClickTicks = 0;
                        pContext->state = BUTTONS_DRIVER_STATE_WAIT_SECOND_CLICK;
                    }
                    else
                    {
                        // Второе нажатие отключено — сразу выдаём SHORT
                        ButtonsDriver_SetPendingEvent(buttonId, BUTTONS_DRIVER_EVENT_SHORT);
                        pContext->state = BUTTONS_DRIVER_STATE_RELEASED;
                    }
                }
            }
            else
            {
                pContext->state = BUTTONS_DRIVER_STATE_PRESSED;
            }

            break;
        }

        case BUTTONS_DRIVER_STATE_WAIT_SECOND_CLICK:
        {
            pContext->secondClickTicks++;

            if (BUTTON_ON == rawState)
            {
                // Пришло второе нажатие — это DOUBLE
                ButtonsDriver_SetPendingEvent(buttonId, BUTTONS_DRIVER_EVENT_DOUBLE);
                pContext->debounceCounter = 0;
                pContext->state = BUTTONS_DRIVER_STATE_WAIT_RELEASE;
            }
            else if (pContext->secondClickTicks >= BUTTONS_DRIVER_SECOND_CLICK_TICKS)
            {
                // Второе нажатие не пришло — это SHORT
                ButtonsDriver_SetPendingEvent(buttonId, BUTTONS_DRIVER_EVENT_SHORT);
                pContext->state = BUTTONS_DRIVER_STATE_RELEASED;
            }

            break;
        }

        case BUTTONS_DRIVER_STATE_LONG_FIRED:
        {
            if (BUTTON_OFF == rawState)
            {
                pContext->debounceCounter++;

                if (pContext->debounceCounter >= BUTTONS_DRIVER_DEBOUNCE_TICKS)
                {
                    // Кнопка отпущена после долгого удержания
                    pContext->lastHoldDurationMs = currentTimeMs - pContext->holdStartTimeMs;
                    pContext->lastReleaseTimeMs = currentTimeMs;
                    ButtonsDriver_SetPendingEvent(buttonId, BUTTONS_DRIVER_EVENT_HOLD_END);

                    pContext->state = BUTTONS_DRIVER_STATE_RELEASED;
                }
            }
            else
            {
                pContext->debounceCounter = 0;

                if (0u != ButtonsDriver_IsEventEnabled(BUTTONS_DRIVER_EVENT_REPEAT))
                {
                    pContext->repeatTicks++;

                    if (pContext->repeatTicks >= BUTTONS_DRIVER_REPEAT_PERIOD_TICKS)
                    {
                        pContext->repeatTicks = 0;
                        ButtonsDriver_SetPendingEvent(buttonId, BUTTONS_DRIVER_EVENT_REPEAT);
                    }
                }
            }

            break;
        }

        case BUTTONS_DRIVER_STATE_WAIT_RELEASE:
        {
            if (BUTTON_OFF == rawState)
            {
                pContext->debounceCounter++;

                if (pContext->debounceCounter >= BUTTONS_DRIVER_DEBOUNCE_TICKS)
                {
                    pContext->state = BUTTONS_DRIVER_STATE_RELEASED;
                }
            }
            else
            {
                pContext->debounceCounter = 0;
            }

            break;
        }

        default:
        {
            pContext->state = BUTTONS_DRIVER_STATE_RELEASED;

            break;
        }
    }
}

//! \brief Инициализация модуля ButtonsDriver
void ButtonsDriver_Init(void)
{
    buttonsEvents = 0u;
    enabledEvents = BUTTONS_DRIVER_ENABLED_EVENTS_INIT;

    uint8_t buttonsRawState = ButtonsDriver_ReadRawState();
    uint32_t currentTimeMs = UserTimer_GetTimeCounterInMs();

    for (uint8_t buttonIndex = 0; buttonIndex < BUTTONS_DRIVER_BUTTON_COUNT; buttonIndex++)
    {
        buttonsDriverContexts[buttonIndex].debounceCounter    = 0u;
        buttonsDriverContexts[buttonIndex].holdStartTimeMs    = 0u;
        buttonsDriverContexts[buttonIndex].secondClickTicks   = 0u;
        buttonsDriverContexts[buttonIndex].repeatTicks        = 0u;
        buttonsDriverContexts[buttonIndex].lastHoldDurationMs = 0u;
        buttonsDriverContexts[buttonIndex].lastPressTimeMs    = 0u;
        buttonsDriverContexts[buttonIndex].lastReleaseTimeMs  = 0u;

        // Если кнопка уже нажата при старте — считаем её нажатой,
        // но не генерируем ложное событие HOLD_START
        uint8_t buttonBit = (buttonsRawState >> buttonIndex) & BUTTONS_DRIVER_RAW_STATE_BIT_MASK;

        if (buttonBit == 0u)
        {
            buttonsDriverContexts[buttonIndex].state = BUTTONS_DRIVER_STATE_PRESSED;
            buttonsDriverContexts[buttonIndex].holdStartTimeMs = currentTimeMs;
            buttonsDriverContexts[buttonIndex].lastPressTimeMs = currentTimeMs;
        }
        else
        {
            buttonsDriverContexts[buttonIndex].state = BUTTONS_DRIVER_STATE_RELEASED;
        }
    }
}

//! \brief Обработчик тика, вызывать в прерывании таймера
//! \details Период вызова должен быть BUTTONS_DRIVER_TICK_MICROSECONDS мкс
void ButtonsDriver_ProcessTick(void)
{
    uint8_t buttonsRawState = ButtonsDriver_ReadRawState();
    uint32_t currentTimeMs = UserTimer_GetTimeCounterInMs();

    for (uint8_t buttonIndex = 0; buttonIndex < BUTTONS_DRIVER_BUTTON_COUNT; buttonIndex++)
    {
        uint8_t buttonBit = (buttonsRawState >> buttonIndex) & BUTTONS_DRIVER_RAW_STATE_BIT_MASK;
        ButtonState rawState = (buttonBit == 0u) ? BUTTON_ON : BUTTON_OFF;

        ButtonsDriver_ProcessStateMachine((Buttons)buttonIndex, &buttonsDriverContexts[buttonIndex],
                                          rawState, currentTimeMs);
    }
}

//! \brief Получить событие кнопки, вызывать в задаче
//! \param[in] buttonId Идентификатор кнопки
//! \return Событие кнопки или BUTTONS_DRIVER_EVENT_NONE, если событий нет
ButtonsDriverEvent ButtonsDriver_GetEvent(Buttons buttonId)
{
    if (buttonId >= BUTTONS_QUANTITY)
    {
        return BUTTONS_DRIVER_EVENT_NONE;
    }

    uint8_t eventShift = BUTTONS_DRIVER_EVENT_SHIFT(buttonId);

    ButtonsDriverEvent event;

    taskENTER_CRITICAL();

    event = (ButtonsDriverEvent)((buttonsEvents >> eventShift) & BUTTONS_DRIVER_EVENT_MASK);

    buttonsEvents &= (uint32_t)~(BUTTONS_DRIVER_EVENT_MASK << eventShift);

    taskEXIT_CRITICAL();

    return event;
}

//! \brief Установить callback для уведомления задачи из прерывания
//! \param[in] pCallback Указатель на функцию-callback (может быть NULL)
void ButtonsDriver_SetEventCallback(void (*pCallback)(void))
{
    pButtonsDriverEventCallback = pCallback;
}

//! \brief Установить маску разрешённых событий
//! \param[in] eventsMask Маска разрешённых событий (комбинация BUTTONS_DRIVER_ENABLE_*)
void ButtonsDriver_SetEnabledEvents(uint8_t eventsMask)
{
    // Короткое нажатие всегда разрешено, независимо от того, что передали
    enabledEvents = (uint8_t)(eventsMask | BUTTONS_DRIVER_ENABLE_SHORT);
}

//! \brief Получить текущую маску разрешённых событий
//! \return Маска разрешённых событий (комбинация BUTTONS_DRIVER_ENABLE_*)
uint8_t ButtonsDriver_GetEnabledEvents(void)
{
    return enabledEvents;
}

//! \brief Получить время последнего удержания кнопки в миллисекундах
//! \param[in] buttonId Идентификатор кнопки
//! \return Время удержания в миллисекундах (0, если не было удержания)
uint32_t ButtonsDriver_GetLastHoldDurationMs(Buttons buttonId)
{
    if (buttonId >= BUTTONS_QUANTITY)
    {
        return 0u;
    }

    return buttonsDriverContexts[buttonId].lastHoldDurationMs;
}

//! \brief Получить момент последнего нажатия кнопки в миллисекундах
//! \param[in] buttonId Идентификатор кнопки
//! \return Момент нажатия в миллисекундах с момента запуска программы
uint32_t ButtonsDriver_GetPressTimestampMs(Buttons buttonId)
{
    if (buttonId >= BUTTONS_QUANTITY)
    {
        return 0u;
    }

    return buttonsDriverContexts[buttonId].lastPressTimeMs;
}

//! \brief Получить момент последнего отпускания кнопки в миллисекундах
//! \param[in] buttonId Идентификатор кнопки
//! \return Момент отпускания в миллисекундах с момента запуска программы
uint32_t ButtonsDriver_GetReleaseTimestampMs(Buttons buttonId)
{
    if (buttonId >= BUTTONS_QUANTITY)
    {
        return 0u;
    }

    return buttonsDriverContexts[buttonId].lastReleaseTimeMs;
}

//! \brief Получить текущее состояние конечного автомата кнопки
//! \param[in] buttonId Идентификатор кнопки
//! \return Состояние конечного автомата кнопки
ButtonsDriverState ButtonsDriver_GetState(Buttons buttonId)
{
    if (buttonId >= BUTTONS_QUANTITY)
    {
        return BUTTONS_DRIVER_STATE_RELEASED;
    }

    return buttonsDriverContexts[buttonId].state;
}

//! \brief Получить маску кнопок, которые считаются нажатыми (по FSM)
//! \return Битовая маска нажатых кнопок (1 = нажата)
uint8_t ButtonsDriver_GetPressedMask(void)
{
    uint8_t pressedMask = 0u;

    taskENTER_CRITICAL();

    for (uint8_t buttonIndex = 0; buttonIndex < BUTTONS_DRIVER_BUTTON_COUNT; buttonIndex++)
    {
        if (0u != ButtonsDriver_IsStatePressed(buttonsDriverContexts[buttonIndex].state))
        {
            pressedMask |= (uint8_t)(1u << buttonIndex);
        }
    }

    taskEXIT_CRITICAL();

    return pressedMask;
}

//! \brief Сбросить состояние одной кнопки
//! \param[in] buttonId Идентификатор кнопки
void ButtonsDriver_ResetButton(Buttons buttonId)
{
    if (buttonId >= BUTTONS_QUANTITY)
    {
        return;
    }

    taskENTER_CRITICAL();

    buttonsDriverContexts[buttonId].state              = BUTTONS_DRIVER_STATE_RELEASED;
    buttonsDriverContexts[buttonId].debounceCounter    = 0u;
    buttonsDriverContexts[buttonId].holdStartTimeMs    = 0u;
    buttonsDriverContexts[buttonId].secondClickTicks   = 0u;
    buttonsDriverContexts[buttonId].repeatTicks        = 0u;
    buttonsDriverContexts[buttonId].lastHoldDurationMs = 0u;
    buttonsDriverContexts[buttonId].lastPressTimeMs    = 0u;
    buttonsDriverContexts[buttonId].lastReleaseTimeMs  = 0u;

    buttonsEvents &= (uint32_t)~(BUTTONS_DRIVER_EVENT_MASK << BUTTONS_DRIVER_EVENT_SHIFT(buttonId));

    taskEXIT_CRITICAL();
}

//! \brief Сбросить состояние всех кнопок
void ButtonsDriver_ResetAll(void)
{
    taskENTER_CRITICAL();

    for (uint8_t buttonIndex = 0; buttonIndex < BUTTONS_DRIVER_BUTTON_COUNT; buttonIndex++)
    {
        buttonsDriverContexts[buttonIndex].state              = BUTTONS_DRIVER_STATE_RELEASED;
        buttonsDriverContexts[buttonIndex].debounceCounter    = 0u;
        buttonsDriverContexts[buttonIndex].holdStartTimeMs    = 0u;
        buttonsDriverContexts[buttonIndex].secondClickTicks   = 0u;
        buttonsDriverContexts[buttonIndex].repeatTicks        = 0u;
        buttonsDriverContexts[buttonIndex].lastHoldDurationMs = 0u;
        buttonsDriverContexts[buttonIndex].lastPressTimeMs    = 0u;
        buttonsDriverContexts[buttonIndex].lastReleaseTimeMs  = 0u;
    }

    buttonsEvents = 0u;

    taskEXIT_CRITICAL();
}
