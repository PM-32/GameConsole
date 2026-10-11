#include "FreeRTOS.h"
#include "task.h"

#include "LedsDriver.h"
#include "LedsEffects.h"
#include "LedsEffectsControl.h"

#define LEDS_EFFECTS_CONTROL_TASK_STACK_SIZE        (configMINIMAL_STACK_SIZE)
#define LEDS_EFFECTS_CONTROL_TASK_PRIORITY          (tskIDLE_PRIORITY + 1u)

#define LEDS_EFFECTS_CONTROL_TRANSFER_TIMEOUT_MS    300u    //!< Таймаут ожидания передачи DMA, мс
#define LEDS_EFFECTS_CONTROL_EFFECT_DURATION_MS     4000u   //!< Длительность одного теста, мс
#define LEDS_EFFECTS_CONTROL_DARKNESS_DURATION_MS   1000u   //!< Длительность паузы между тестами, мс
#define LEDS_EFFECTS_CONTROL_UPDATE_PERIOD_MS       LEDS_EFFECTS_UPDATE_PERIOD_MS //!< Период обновления, мс
#define LEDS_EFFECTS_CONTROL_BRIGHTNESS_PERCENT     3u      //!< Яркость ленты для тестового режима, %

#define LEDS_EFFECTS_CONTROL_SOLID_RED              0u      //!< SOLID: красная компонента
#define LEDS_EFFECTS_CONTROL_SOLID_GREEN            255u    //!< SOLID: зелёная компонента
#define LEDS_EFFECTS_CONTROL_SOLID_BLUE             0u      //!< SOLID: синяя компонента

#define LEDS_EFFECTS_CONTROL_GRADIENT_RED1          255u    //!< GRADIENT: цвет 1 — красный
#define LEDS_EFFECTS_CONTROL_GRADIENT_GREEN1        0u      //!< GRADIENT: цвет 1 — зелёная компонента
#define LEDS_EFFECTS_CONTROL_GRADIENT_BLUE1         0u      //!< GRADIENT: цвет 1 — синяя компонента
#define LEDS_EFFECTS_CONTROL_GRADIENT_RED2          0u      //!< GRADIENT: цвет 2 — красная компонента
#define LEDS_EFFECTS_CONTROL_GRADIENT_GREEN2        0u      //!< GRADIENT: цвет 2 — зелёная компонента
#define LEDS_EFFECTS_CONTROL_GRADIENT_BLUE2         255u    //!< GRADIENT: цвет 2 — синий

#define LEDS_EFFECTS_CONTROL_CYCLE_RED1             255u    //!< CYCLE: цвет 1 — жёлтый (R)
#define LEDS_EFFECTS_CONTROL_CYCLE_GREEN1           255u    //!< CYCLE: цвет 1 — жёлтый (G)
#define LEDS_EFFECTS_CONTROL_CYCLE_BLUE1            0u      //!< CYCLE: цвет 1 — жёлтый (B)
#define LEDS_EFFECTS_CONTROL_CYCLE_RED2             255u    //!< CYCLE: цвет 2 — пурпурный (R)
#define LEDS_EFFECTS_CONTROL_CYCLE_GREEN2           0u      //!< CYCLE: цвет 2 — пурпурный (G)
#define LEDS_EFFECTS_CONTROL_CYCLE_BLUE2            255u    //!< CYCLE: цвет 2 — пурпурный (B)

#define LEDS_EFFECTS_CONTROL_COLOR_MODES_QUANTITY   5u      //!< Количество тестируемых colorMode

#define LEDS_EFFECTS_CONTROL_BLINK_PERIOD_MS        500u    //!< Период Blink, мс
#define LEDS_EFFECTS_CONTROL_PULSE_PERIOD_MS        1000u   //!< Период Pulse, мс
#define LEDS_EFFECTS_CONTROL_RAINBOW_PERIOD_MS      3000u   //!< Период Rainbow, мс
#define LEDS_EFFECTS_CONTROL_KNIGHT_RIDER_PERIOD_MS 1000u   //!< Период Knight Rider, мс
#define LEDS_EFFECTS_CONTROL_COLOR_CYCLE_PERIOD_MS  3000u   //!< Период Color Cycle, мс
#define LEDS_EFFECTS_CONTROL_CHASE_PERIOD_MS        1500u   //!< Период Chase, мс
#define LEDS_EFFECTS_CONTROL_SPARKLE_PERIOD_MS      2000u   //!< Период Sparkle, мс
#define LEDS_EFFECTS_CONTROL_THEATER_PERIOD_MS      1000u   //!< Период Theater Chase, мс

static TaskHandle_t ledsEffectsControlTaskHandle = NULL;    //!< Хэндл задачи тестирования

//! \brief Список тестируемых colorMode
static const LedsEffectsColorMode testColorModes[LEDS_EFFECTS_CONTROL_COLOR_MODES_QUANTITY] =
{
    LEDS_EFFECTS_COLOR_SOLID,
    LEDS_EFFECTS_COLOR_RAINBOW,
    LEDS_EFFECTS_COLOR_GRADIENT,
    LEDS_EFFECTS_COLOR_MODE_CYCLE,
    LEDS_EFFECTS_COLOR_RANDOM
};

//! \brief Callback завершения передачи DMA (вызывается из ISR)
static void LedsEffectsControl_OnTransferComplete(void)
{
    BaseType_t higherPriorityTaskWoken = pdFALSE;

    if (NULL != ledsEffectsControlTaskHandle)
    {
        vTaskNotifyGiveFromISR(ledsEffectsControlTaskHandle, &higherPriorityTaskWoken);
        portYIELD_FROM_ISR(higherPriorityTaskWoken);
    }
}

//! \brief Применение эффекта к ленте и ожидание завершения передачи
static void LedsEffectsControl_ShowAndWait(void)
{
    LedsDriver_Show();
    ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(LEDS_EFFECTS_CONTROL_TRANSFER_TIMEOUT_MS));
}

//! \brief Гашение ленты и ожидание завершения передачи
static void LedsEffectsControl_ClearAndWait(void)
{
    LedsDriver_Clear();
    LedsEffectsControl_ShowAndWait();
}

//! \brief Проверка, игнорирует ли эффект colorMode
//! \param[in] type Тип эффекта
//! \return 1 - игнорирует, 0 - использует
static uint8_t LedsEffectsControl_EffectIgnoresColorMode(LedsEffectsType type)
{
    return (LEDS_EFFECTS_RAINBOW == type) || (LEDS_EFFECTS_COLOR_CYCLE == type);
}

//! \brief Получение периода для типа эффекта
//! \param[in] type Тип эффекта
//! \return Период, мс (0 — не используется)
static uint16_t LedsEffectsControl_GetPeriodMs(LedsEffectsType type)
{
    switch (type)
    {
        case LEDS_EFFECTS_BLINK:
        {
            return LEDS_EFFECTS_CONTROL_BLINK_PERIOD_MS;
        }

        case LEDS_EFFECTS_PULSE:
        {
            return LEDS_EFFECTS_CONTROL_PULSE_PERIOD_MS;
        }

        case LEDS_EFFECTS_RAINBOW:
        {
            return LEDS_EFFECTS_CONTROL_RAINBOW_PERIOD_MS;
        }

        case LEDS_EFFECTS_KNIGHT_RIDER:
        {
            return LEDS_EFFECTS_CONTROL_KNIGHT_RIDER_PERIOD_MS;
        }

        case LEDS_EFFECTS_COLOR_CYCLE:
        {
            return LEDS_EFFECTS_CONTROL_COLOR_CYCLE_PERIOD_MS;
        }

        case LEDS_EFFECTS_CHASE:
        {
            return LEDS_EFFECTS_CONTROL_CHASE_PERIOD_MS;
        }

        case LEDS_EFFECTS_SPARKLE:
        {
            return LEDS_EFFECTS_CONTROL_SPARKLE_PERIOD_MS;
        }

        case LEDS_EFFECTS_THEATER_CHASE:
        {
            return LEDS_EFFECTS_CONTROL_THEATER_PERIOD_MS;
        }

        default:
        {
            return 0u;
        }
    }
}

//! \brief Формирование параметров теста по типу и режиму цвета
//! \param[in]  type      Тип эффекта
//! \param[in]  colorMode Режим цвета
//! \param[out] pParams   Параметры эффекта
static void LedsEffectsControl_FillParams(LedsEffectsType type,
                                          LedsEffectsColorMode colorMode,
                                          LedsEffectsParams *pParams)
{
    pParams->type = type;
    pParams->priority = LEDS_EFFECTS_PRIORITY_NORMAL;
    pParams->colorMode = colorMode;
    pParams->durationMs = LEDS_EFFECTS_CONTROL_EFFECT_DURATION_MS;
    pParams->periodMs = LedsEffectsControl_GetPeriodMs(type);
    pParams->count = 0u;

    pParams->red1 = LEDS_EFFECTS_CONTROL_SOLID_RED;
    pParams->green1 = LEDS_EFFECTS_CONTROL_SOLID_GREEN;
    pParams->blue1 = LEDS_EFFECTS_CONTROL_SOLID_BLUE;
    pParams->red2 = 0u;
    pParams->green2 = 0u;
    pParams->blue2 = 0u;

    switch (colorMode)
    {
        case LEDS_EFFECTS_COLOR_GRADIENT:
        {
            pParams->red1 = LEDS_EFFECTS_CONTROL_GRADIENT_RED1;
            pParams->green1 = LEDS_EFFECTS_CONTROL_GRADIENT_GREEN1;
            pParams->blue1 = LEDS_EFFECTS_CONTROL_GRADIENT_BLUE1;
            pParams->red2 = LEDS_EFFECTS_CONTROL_GRADIENT_RED2;
            pParams->green2 = LEDS_EFFECTS_CONTROL_GRADIENT_GREEN2;
            pParams->blue2 = LEDS_EFFECTS_CONTROL_GRADIENT_BLUE2;

            break;
        }

        case LEDS_EFFECTS_COLOR_MODE_CYCLE:
        {
            pParams->red1 = LEDS_EFFECTS_CONTROL_CYCLE_RED1;
            pParams->green1 = LEDS_EFFECTS_CONTROL_CYCLE_GREEN1;
            pParams->blue1 = LEDS_EFFECTS_CONTROL_CYCLE_BLUE1;
            pParams->red2 = LEDS_EFFECTS_CONTROL_CYCLE_RED2;
            pParams->green2 = LEDS_EFFECTS_CONTROL_CYCLE_GREEN2;
            pParams->blue2 = LEDS_EFFECTS_CONTROL_CYCLE_BLUE2;

            break;
        }

        default:
        {
            break;
        }
    }
}

//! \brief Выполнение одного теста (эффект + colorMode)
//! \param[in] pParams Параметры эффекта
static void LedsEffectsControl_RunEffect(const LedsEffectsParams *pParams)
{
    LedsEffects_Start(pParams);

    uint32_t elapsedMs = 0u;

    while (elapsedMs < LEDS_EFFECTS_CONTROL_EFFECT_DURATION_MS)
    {
        LedsEffects_Update();
        LedsEffectsControl_ShowAndWait();

        vTaskDelay(pdMS_TO_TICKS(LEDS_EFFECTS_CONTROL_UPDATE_PERIOD_MS));

        elapsedMs += LEDS_EFFECTS_CONTROL_UPDATE_PERIOD_MS;
    }

    LedsEffects_Stop();
}

//! \brief Задача тестирования световых эффектов
//! \param[in] pvParameters Параметры задачи (не используются)
static void LedsEffectsControl_Task(void *pvParameters)
{
    (void)pvParameters;

    ledsEffectsControlTaskHandle = xTaskGetCurrentTaskHandle();

    LedsDriver_SetTransferCompleteCallback(LedsEffectsControl_OnTransferComplete);
    LedsDriver_SetBrightness(LEDS_EFFECTS_CONTROL_BRIGHTNESS_PERCENT);

    for (;;)
    {
        for (LedsEffectsType type = LEDS_EFFECTS_FADE_IN;
             type <= LEDS_EFFECTS_THEATER_CHASE;
             type++)
        {
            for (uint8_t modeIndex = 0u; modeIndex < LEDS_EFFECTS_CONTROL_COLOR_MODES_QUANTITY; modeIndex++)
            {
                LedsEffectsColorMode colorMode = testColorModes[modeIndex];

                if ((0u != LedsEffectsControl_EffectIgnoresColorMode(type)) &&
                    (LEDS_EFFECTS_COLOR_SOLID != colorMode))
                {
                    continue;
                }

                LedsEffectsParams params = { 0 };

                LedsEffectsControl_FillParams(type, colorMode, &params);
                LedsEffectsControl_RunEffect(&params);

                LedsEffectsControl_ClearAndWait();
                vTaskDelay(pdMS_TO_TICKS(LEDS_EFFECTS_CONTROL_DARKNESS_DURATION_MS));
            }
        }
    }
}

//! \brief Создание задачи тестирования световых эффектов
//! \return Результат (статус) создания задачи
BaseType_t LedsEffectsControl_CreateTask(void)
{
    BaseType_t xTaskCreationResult = xTaskCreate(LedsEffectsControl_Task, "LedsEff",
                                                 LEDS_EFFECTS_CONTROL_TASK_STACK_SIZE, NULL,
                                                 LEDS_EFFECTS_CONTROL_TASK_PRIORITY,
                                                 &ledsEffectsControlTaskHandle);

    return xTaskCreationResult;
}
