#include <stdint.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"
#include "stm32f1xx_hal.h"

#include "Hardware.h"
#include "LedsDriver.h"

#define CCR_ZERO                    30u     //!< Значение CCR для логического «0» (0.375 мкс)
#define CCR_ONE                     60u     //!< Значение CCR для логической «1» (0.75 мкс)

#define BITS_PER_COMPONENT          8u                                              //!< Количество бит в одной цветовой компоненте
#define COLOR_COMPONENTS_QUANTITY   3u                                              //!< Количество цветовых компонент (G, R, B)
#define BITS_PER_LED                (BITS_PER_COMPONENT * COLOR_COMPONENTS_QUANTITY) //!< Количество бит на один светодиод

#define RESET_SLOTS                 40u                                             //!< Количество слотов CCR=0 для reset-паузы (40 * 1.25 мкс = 50 мкс)
#define DMA_BUFFER_SIZE             (LEDS_QUANTITY * BITS_PER_LED + RESET_SLOTS)    //!< Размер DMA-буфера

#define MAX_PERCENT                 100u    //!< Максимальное значение яркости, %
#define MAX_COMPONENT_VALUE         255u    //!< Максимальное значение цветовой компоненты
#define HIGHEST_BIT_MASK            0x80u   //!< Маска старшего бита в байте

#define RAINBOW_HUE_CIRCLE          256u                                            //!< Полный круг оттенков (0-255)
#define RAINBOW_SECTORS_QUANTITY    6u                                              //!< Количество секторов цветового круга
#define RAINBOW_SECTOR_SIZE         (RAINBOW_HUE_CIRCLE / RAINBOW_SECTORS_QUANTITY) //!< Размер одного сектора цветового круга
#define RAINBOW_FRACTION_MAX        255u                                            //!< Максимальное значение дроби внутри сектора

static LedState ledsState[LEDS_QUANTITY] = { 0 };       //!< Состояние всех светодиодов
static LedState savedLedsState[LEDS_QUANTITY] = { 0 };  //!< Сохранённое состояние для Sleep/Wake
static uint16_t dmaBuffer[DMA_BUFFER_SIZE] = { 0 };     //!< Буфер DMA для передачи битов WS2812B
static uint8_t brightnessPercent = MAX_PERCENT;         //!< Текущая яркость, %
static volatile uint8_t transferInProgress = 0;         //!< Флаг активной передачи DMA

static void (*pTransferCompleteCallback)(void) = NULL;  //!< Callback по завершении передачи

//! \brief Масштабирование компоненты цвета яркостью
//! \param[in] component - исходная компонента цвета
//! \return Масштабированная компонента цвета
static uint8_t LedsDriver_ScaleComponent(uint8_t component)
{
    uint16_t scaledComponent = ((uint16_t)component * brightnessPercent) / MAX_PERCENT;

    if (scaledComponent > MAX_COMPONENT_VALUE)
    {
        scaledComponent = MAX_COMPONENT_VALUE;
    }

    return (uint8_t)scaledComponent;
}

//! \brief Заполнение битов одной компоненты цвета в DMA-буфере
//! \param[in] pBuffer - указатель на DMA-буфер
//! \param[in] pBufferIndex - указатель на текущий индекс в буфере
//! \param[in] component - компонента цвета
static void LedsDriver_FillComponentBits(uint16_t *pBuffer, uint16_t *pBufferIndex, uint8_t component)
{
    for (uint8_t bitIndex = 0; bitIndex < BITS_PER_COMPONENT; bitIndex++)
    {
        if (component & (HIGHEST_BIT_MASK >> bitIndex))
        {
            pBuffer[*pBufferIndex] = CCR_ONE;
        }
        else
        {
            pBuffer[*pBufferIndex] = CCR_ZERO;
        }

        (*pBufferIndex)++;
    }
}

//! \brief Формирование DMA-буфера из ledsState
static void LedsDriver_FillDmaBuffer(void)
{
    uint16_t bufferIndex = 0;

    for (uint8_t ledIndex = 0; ledIndex < LEDS_QUANTITY; ledIndex++)
    {
        // Порядок полей LedState (green, red, blue) соответствует порядку
        // передачи WS2812B. Приведение к uint8_t * корректно, так как
        // структура не содержит битовых полей и выравнивания.
        uint8_t *pComponents = (uint8_t *)&ledsState[ledIndex];

        for (uint8_t componentIndex = 0; componentIndex < COLOR_COMPONENTS_QUANTITY; componentIndex++)
        {
            uint8_t component = pComponents[componentIndex];

            // Масштабирование яркостью
            component = LedsDriver_ScaleComponent(component);

            // Заполнение битов компоненты
            LedsDriver_FillComponentBits(dmaBuffer, &bufferIndex, component);
        }
    }

    // Заполнение reset-паузы нулевыми значениями CCR
    for (uint8_t resetIndex = 0; resetIndex < RESET_SLOTS; resetIndex++)
    {
        dmaBuffer[bufferIndex] = 0u;
        bufferIndex++;
    }
}

//! \brief Принудительная остановка активной передачи DMA
static void LedsDriver_StopTransfer(void)
{
    if (transferInProgress != 0u)
    {
        HAL_TIM_PWM_Stop_DMA(&TIM_PWM_GENERATION_HTIM, TIM_PWM_GENERATION_CHANNEL);
        transferInProgress = 0;
    }
}

//! \brief Инициализация драйвера светодиодов
void LedsDriver_Init(void)
{
    // Если модуль уже инициализирован — повторная инициализация не требуется
    static uint8_t initialized = 0;

    if (initialized != 0u)
    {
        return;
    }

    // Сброс состояния светодиодов
    memset(ledsState, 0, sizeof(ledsState));
    memset(savedLedsState, 0, sizeof(savedLedsState));
    memset(dmaBuffer, 0, sizeof(dmaBuffer));

    brightnessPercent = MAX_PERCENT;
    transferInProgress = 0;
    pTransferCompleteCallback = NULL;

    initialized = 1;
}

//! \brief Установка цвета одного светодиода
void LedsDriver_SetColor(uint8_t ledIndex, uint8_t red, uint8_t green, uint8_t blue)
{
    if (ledIndex >= LEDS_QUANTITY)
    {
        return;
    }

    ledsState[ledIndex].red = red;
    ledsState[ledIndex].green = green;
    ledsState[ledIndex].blue = blue;
}

//! \brief Установка цвета всех светодиодов
void LedsDriver_SetAll(uint8_t red, uint8_t green, uint8_t blue)
{
    for (uint8_t ledIndex = 0; ledIndex < LEDS_QUANTITY; ledIndex++)
    {
        LedsDriver_SetColor(ledIndex, red, green, blue);
    }
}

//! \brief Гашение всех светодиодов
void LedsDriver_Clear(void)
{
    LedsDriver_SetAll(0u, 0u, 0u);
}

//! \brief Чтение текущего цвета светодиода
void LedsDriver_GetColor(uint8_t ledIndex, uint8_t *pRed, uint8_t *pGreen, uint8_t *pBlue)
{
    if (ledIndex >= LEDS_QUANTITY)
    {
        return;
    }

    if (pRed != NULL)
    {
        *pRed = ledsState[ledIndex].red;
    }

    if (pGreen != NULL)
    {
        *pGreen = ledsState[ledIndex].green;
    }

    if (pBlue != NULL)
    {
        *pBlue = ledsState[ledIndex].blue;
    }
}

//! \brief Получение состояния одного светодиода
void LedsDriver_GetLedState(uint8_t ledIndex, LedState *pLedState)
{
    if ((ledIndex >= LEDS_QUANTITY) || (pLedState == NULL))
    {
        return;
    }

    *pLedState = ledsState[ledIndex];
}

//! \brief Применение буфера к ленте (запуск передачи через DMA)
void LedsDriver_Show(void)
{
    // Критическая секция защищает проверку и установку флага
    // от гонки с ISR завершения передачи
    taskENTER_CRITICAL();

    if (transferInProgress != 0u)
    {
        taskEXIT_CRITICAL();
        return;
    }

    transferInProgress = 1;

    taskEXIT_CRITICAL();

    // Формирование DMA-буфера из ledsState
    LedsDriver_FillDmaBuffer();

    // Сброс счётчика таймера, чтобы первый CCR применился
    // в начале нового периода (иначе первый бит кадра искажается)
    __HAL_TIM_SET_COUNTER(&TIM_PWM_GENERATION_HTIM, 0u);

    // HAL API ожидает uint32_t *, но на STM32F1 регистр CCR 16-битный,
    // а DMA настроен на HALFWORD (см. tim.c). Приведение корректно.
    HAL_StatusTypeDef status = HAL_TIM_PWM_Start_DMA(&TIM_PWM_GENERATION_HTIM,
                                                     TIM_PWM_GENERATION_CHANNEL,
                                                     (uint32_t *)dmaBuffer,
                                                     DMA_BUFFER_SIZE);

    // При ошибке запуска DMA сбрасываем флаг, иначе модуль «залипнет»
    if (status != HAL_OK)
    {
        transferInProgress = 0;
    }
}

//! \brief Установка яркости в процентах
void LedsDriver_SetBrightness(uint8_t percent)
{
    if (percent > MAX_PERCENT)
    {
        percent = MAX_PERCENT;
    }

#if (LEDS_SAFE_BRIGHTNESS_ENABLE == 1u)
    if (percent > LEDS_SAFE_BRIGHTNESS_MAX_PERCENT)
    {
        percent = LEDS_SAFE_BRIGHTNESS_MAX_PERCENT;
    }
#endif

    brightnessPercent = percent;
}

//! \brief Чтение текущей яркости
uint8_t LedsDriver_GetBrightness(void)
{
    return brightnessPercent;
}

//! \brief Перевод ленты в режим пониженного потребления
void LedsDriver_Sleep(void)
{
    // Принудительная остановка активной передачи
    LedsDriver_StopTransfer();

    // Сохранение текущего состояния
    for (uint8_t ledIndex = 0; ledIndex < LEDS_QUANTITY; ledIndex++)
    {
        savedLedsState[ledIndex] = ledsState[ledIndex];
    }

    // Запись нулей во все светодиоды
    LedsDriver_Clear();

    // Применение состояния к ленте
    LedsDriver_Show();
}

//! \brief Восстановление ленты после режима пониженного потребления
void LedsDriver_Wake(void)
{
    // Принудительная остановка активной передачи
    LedsDriver_StopTransfer();

    // Восстановление сохранённого состояния
    for (uint8_t ledIndex = 0; ledIndex < LEDS_QUANTITY; ledIndex++)
    {
        ledsState[ledIndex] = savedLedsState[ledIndex];
    }

    // Применение состояния к ленте
    LedsDriver_Show();
}

//! \brief Установка callback по завершении передачи
void LedsDriver_SetTransferCompleteCallback(void (*pCallback)(void))
{
    pTransferCompleteCallback = pCallback;
}

//! \brief Установка «радуги» для ambient-режима
void LedsDriver_SetRainbow(uint8_t offset)
{
    for (uint8_t ledIndex = 0; ledIndex < LEDS_QUANTITY; ledIndex++)
    {
        // Равномерное распределение оттенков по ленте
        uint8_t hue = (uint8_t)((uint16_t)offset + ((uint16_t)ledIndex * RAINBOW_HUE_CIRCLE) / LEDS_QUANTITY);

        // Разбиение круга на секторы
        uint8_t sector = hue / RAINBOW_SECTOR_SIZE;
        uint8_t position = hue % RAINBOW_SECTOR_SIZE;

        // Масштабирование позиции внутри сектора
        uint8_t fraction = (uint8_t)(((uint16_t)position * RAINBOW_FRACTION_MAX) / RAINBOW_SECTOR_SIZE);

        uint8_t red = 0;
        uint8_t green = 0;
        uint8_t blue = 0;

        switch (sector)
        {
            case 0:     // Красный -> жёлтый
            {
                red = MAX_COMPONENT_VALUE;
                green = fraction;
                blue = 0u;

                break;
            }

            case 1:     // Жёлтый -> зелёный
            {
                red = MAX_COMPONENT_VALUE - fraction;
                green = MAX_COMPONENT_VALUE;
                blue = 0u;

                break;
            }

            case 2:     // Зелёный -> голубой
            {
                red = 0u;
                green = MAX_COMPONENT_VALUE;
                blue = fraction;

                break;
            }

            case 3:     // Голубой -> синий
            {
                red = 0u;
                green = MAX_COMPONENT_VALUE - fraction;
                blue = MAX_COMPONENT_VALUE;

                break;
            }

            case 4:     // Синий -> пурпурный
            {
                red = fraction;
                green = 0u;
                blue = MAX_COMPONENT_VALUE;

                break;
            }

            case 5:     // Пурпурный -> красный
            {
                red = MAX_COMPONENT_VALUE;
                green = 0u;
                blue = MAX_COMPONENT_VALUE - fraction;

                break;
            }

            default:
            {
                red = 0u;
                green = 0u;
                blue = 0u;

                break;
            }
        }

        LedsDriver_SetColor(ledIndex, red, green, blue);
    }
}

//! \brief Callback завершения передачи DMA (для HAL_TIM_PWM_PulseFinishedCallback)
//! \details Вызывается из SystemCallbacks.c при завершении генерации ШИМ.
void LedsDriver_TransferCompleteCallback(void)
{
    // Принудительно останавливаем ШИМ и DMA, чтобы таймер TIM4
    // не генерировал сигнал после окончания кадра.
    // Это гарантирует, что на DIN будет низкий уровень до следующего кадра.
    HAL_TIM_PWM_Stop_DMA(&TIM_PWM_GENERATION_HTIM, TIM_PWM_GENERATION_CHANNEL);

    transferInProgress = 0;

    if (pTransferCompleteCallback != NULL)
    {
        pTransferCompleteCallback();
    }
}
