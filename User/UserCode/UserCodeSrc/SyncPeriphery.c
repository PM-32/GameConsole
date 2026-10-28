#include <stdint.h>
#include <string.h>

#include "SyncPeriphery.h"

#include "BuzzersDriver.h"
#include "LedsDriver.h"
#include "LedsEffects.h"

//! Максимальное значение цветовой компоненты
#define MAX_COMPONENT_VALUE             255u

//! Количество нот в таблице маппинга (C4–B5, две октавы)
#define FREQUENCY_COLOR_MAP_SIZE        24u

//! \brief Элемент таблицы маппинга частоты в цвет
typedef struct
{
    uint16_t frequencyHz;   //!< Частота ноты, Гц
    uint8_t  red;           //!< Красная компонента
    uint8_t  green;         //!< Зелёная компонента
    uint8_t  blue;          //!< Синяя компонента
} FrequencyColorEntry;

//! Таблица маппинга частот в цвета (C4–B5, без интерполяции)
static const FrequencyColorEntry frequencyColorMap[FREQUENCY_COLOR_MAP_SIZE] =
{
    { 262u, 255u,   0u,   0u },   //!< C4  — красный
    { 277u, 255u,   0u,   0u },   //!< C#4 — красный
    { 294u, 255u, 128u,   0u },   //!< D4  — оранжевый
    { 311u, 255u, 128u,   0u },   //!< D#4 — оранжевый
    { 330u, 255u, 255u,   0u },   //!< E4  — жёлтый
    { 349u,   0u, 255u,   0u },   //!< F4  — зелёный
    { 370u,   0u, 255u,   0u },   //!< F#4 — зелёный
    { 392u,   0u, 255u, 255u },   //!< G4  — голубой
    { 415u,   0u, 255u, 255u },   //!< G#4 — голубой
    { 440u,   0u,   0u, 255u },   //!< A4  — синий
    { 466u,   0u,   0u, 255u },   //!< A#4 — синий
    { 494u, 128u,   0u, 255u },   //!< B4  — фиолетовый

    { 523u, 255u,   0u,   0u },   //!< C5  — красный
    { 554u, 255u,   0u,   0u },   //!< C#5 — красный
    { 587u, 255u, 128u,   0u },   //!< D5  — оранжевый
    { 622u, 255u, 128u,   0u },   //!< D#5 — оранжевый
    { 659u, 255u, 255u,   0u },   //!< E5  — жёлтый
    { 698u,   0u, 255u,   0u },   //!< F5  — зелёный
    { 740u,   0u, 255u,   0u },   //!< F#5 — зелёный
    { 784u,   0u, 255u, 255u },   //!< G5  — голубой
    { 831u,   0u, 255u, 255u },   //!< G#5 — голубой
    { 880u,   0u,   0u, 255u },   //!< A5  — синий
    { 932u,   0u,   0u, 255u },   //!< A#5 — синий
    { 988u, 128u,   0u, 255u }    //!< B5  — фиолетовый
};

//! Массив вспышек
static SyncPeripheryFlash flashes[SYNC_PERIPHERY_FLASH_COUNT_MAX] = { 0 };

//! Флаг инициализации модуля
static uint8_t initialized = 0u;

//! \brief Преобразование частоты в цвет
void SyncPeriphery_FrequencyToColor(uint16_t frequencyHz,
                                    uint8_t *pRed, uint8_t *pGreen, uint8_t *pBlue)
{
    if ((pRed == NULL) || (pGreen == NULL) || (pBlue == NULL))
    {
        return;
    }

    uint8_t  found = 0u;
    uint32_t bestDiff = 0xFFFFFFFFu;
    uint8_t  bestIndex = 0u;

    for (uint8_t index = 0u; index < FREQUENCY_COLOR_MAP_SIZE; index++)
    {
        uint32_t mapFreq = frequencyColorMap[index].frequencyHz;

        if (frequencyHz >= mapFreq)
        {
            uint32_t diff = frequencyHz - mapFreq;

            if (diff < bestDiff)
            {
                bestDiff = diff;
                bestIndex = index;
                found = 1u;
            }
        }
        else
        {
            uint32_t diff = mapFreq - frequencyHz;

            if (diff < bestDiff)
            {
                bestDiff = diff;
                bestIndex = index;
                found = 1u;
            }
        }
    }

    if ((0u != found) && (bestDiff <= SYNC_PERIPHERY_FREQ_TOLERANCE_HZ))
    {
        *pRed   = frequencyColorMap[bestIndex].red;
        *pGreen = frequencyColorMap[bestIndex].green;
        *pBlue  = frequencyColorMap[bestIndex].blue;

        return;
    }

    *pRed   = MAX_COMPONENT_VALUE;
    *pGreen = MAX_COMPONENT_VALUE;
    *pBlue  = MAX_COMPONENT_VALUE;
}

//! \brief Запуск вспышки на одном LED
void SyncPeriphery_StartFlash(uint8_t ledIndex,
                              uint8_t red, uint8_t green, uint8_t blue,
                              uint16_t durationMs)
{
    if (ledIndex >= LEDS_QUANTITY)
    {
        return;
    }

    if (0u == durationMs)
    {
        return;
    }

    flashes[ledIndex].active      = 1u;
    flashes[ledIndex].red         = red;
    flashes[ledIndex].green       = green;
    flashes[ledIndex].blue        = blue;
    flashes[ledIndex].remainingMs = durationMs;
    flashes[ledIndex].totalMs     = durationMs;
}

//! \brief Остановка всех вспышек
void SyncPeriphery_StopAllFlashes(void)
{
    for (uint8_t index = 0u; index < SYNC_PERIPHERY_FLASH_COUNT_MAX; index++)
    {
        if (0u != flashes[index].active)
        {
            flashes[index].active      = 0u;
            flashes[index].remainingMs = 0u;
            flashes[index].totalMs     = 0u;

            LedsDriver_SetColor(index, 0u, 0u, 0u);
        }
    }
}

//! \brief Проверка наличия активных вспышек
uint8_t SyncPeriphery_HasActiveFlashes(void)
{
    for (uint8_t index = 0u; index < SYNC_PERIPHERY_FLASH_COUNT_MAX; index++)
    {
        if (0u != flashes[index].active)
        {
            return 1u;
        }
    }

    return 0u;
}

//! \brief Инициализация модуля SyncPeriphery
void SyncPeriphery_Init(void)
{
    if (0u != initialized)
    {
        return;
    }

    memset(flashes, 0, sizeof(flashes));

    initialized = 1u;
}

//! \brief Обновление состояния вспышек
void SyncPeriphery_Update(void)
{
    if (0u == initialized)
    {
        return;
    }

    for (uint8_t ledIndex = 0u; ledIndex < SYNC_PERIPHERY_FLASH_COUNT_MAX; ledIndex++)
    {
        SyncPeripheryFlash *pFlash = &flashes[ledIndex];

        if (0u == pFlash->active)
        {
            continue;
        }

        if (pFlash->remainingMs > 0u)
        {
            pFlash->remainingMs--;
        }

        if (0u == pFlash->remainingMs)
        {
            pFlash->active = 0u;
            LedsDriver_SetColor(ledIndex, 0u, 0u, 0u);

            continue;
        }

        // Защита от деления на ноль
        if (pFlash->totalMs > 0u)
        {
            uint32_t brightness = ((uint32_t)pFlash->remainingMs * MAX_COMPONENT_VALUE) / pFlash->totalMs;

            uint8_t red   = (uint8_t)(((uint32_t)pFlash->red   * brightness) / MAX_COMPONENT_VALUE);
            uint8_t green = (uint8_t)(((uint32_t)pFlash->green * brightness) / MAX_COMPONENT_VALUE);
            uint8_t blue  = (uint8_t)(((uint32_t)pFlash->blue  * brightness) / MAX_COMPONENT_VALUE);

            LedsDriver_SetColor(ledIndex, red, green, blue);
        }
    }
}

//! \brief Отклик «клик» (кнопка нажата)
void SyncPeriphery_Click(void)
{
    BuzzersDriver_SetVolume(BUZZER_SMALL, SYNC_PERIPHERY_CLICK_VOLUME_PERCENT);
    BuzzersDriver_PlayTone(BUZZER_SMALL, 2000u, SYNC_PERIPHERY_CLICK_DURATION_MS);

    for (uint8_t ledIndex = 0u; ledIndex < LEDS_QUANTITY; ledIndex++)
    {
        SyncPeriphery_StartFlash(ledIndex,
                                 SYNC_PERIPHERY_CLICK_RED,
                                 SYNC_PERIPHERY_CLICK_GREEN,
                                 SYNC_PERIPHERY_CLICK_BLUE,
                                 SYNC_PERIPHERY_CLICK_FLASH_MS);
    }
}

//! \brief Отклик «нота»
void SyncPeriphery_PlayNote(Buzzer buzzer, uint16_t frequencyHz, uint16_t durationMs)
{
    BuzzersDriver_PlayTone(buzzer, frequencyHz, durationMs);

    if (0u == frequencyHz)
    {
        return;
    }

    uint8_t red   = 0u;
    uint8_t green = 0u;
    uint8_t blue  = 0u;

    SyncPeriphery_FrequencyToColor(frequencyHz, &red, &green, &blue);

    for (uint8_t ledIndex = 0u; ledIndex < LEDS_QUANTITY; ledIndex++)
    {
        SyncPeriphery_StartFlash(ledIndex, red, green, blue, durationMs);
    }
}

//! \brief Остановка ноты
void SyncPeriphery_StopNote(Buzzer buzzer)
{
    BuzzersDriver_Stop(buzzer);
}

//! \brief Отклик «успех»
void SyncPeriphery_Success(void)
{
    BuzzersDriver_Success(BUZZER_SMALL);

    LedsEffectsParams params = { 0 };

    params.type       = LEDS_EFFECTS_PROGRESS;
    params.priority   = LEDS_EFFECTS_PRIORITY_NORMAL;
    params.colorMode  = LEDS_EFFECTS_COLOR_SOLID;
    params.red1       = 0u;
    params.green1     = 255u;
    params.blue1      = 0u;
    params.red2       = 0u;
    params.green2     = 0u;
    params.blue2      = 0u;
    params.durationMs = SYNC_PERIPHERY_SUCCESS_FLASH_MS;
    params.periodMs   = 0u;
    params.count      = 0u;

    LedsEffects_Start(&params);
}

//! \brief Отклик «ошибка»
void SyncPeriphery_Error(void)
{
    BuzzersDriver_Error(BUZZER_SMALL);

    for (uint8_t ledIndex = 0u; ledIndex < LEDS_QUANTITY; ledIndex++)
    {
        SyncPeriphery_StartFlash(ledIndex,
                                 SYNC_PERIPHERY_ERROR_RED,
                                 SYNC_PERIPHERY_ERROR_GREEN,
                                 SYNC_PERIPHERY_ERROR_BLUE,
                                 SYNC_PERIPHERY_ERROR_FLASH_MS);
    }
}

//! \brief Отклик «попадание»
void SyncPeriphery_Hit(void)
{
    BuzzersDriver_SetVolume(BUZZER_SMALL, SYNC_PERIPHERY_HIT_VOLUME_PERCENT);
    BuzzersDriver_PlayTone(BUZZER_SMALL,
                           SYNC_PERIPHERY_HIT_FREQUENCY_HZ,
                           SYNC_PERIPHERY_HIT_DURATION_MS);

    for (uint8_t ledIndex = 0u; ledIndex < LEDS_QUANTITY; ledIndex++)
    {
        SyncPeriphery_StartFlash(ledIndex,
                                 SYNC_PERIPHERY_HIT_RED,
                                 SYNC_PERIPHERY_HIT_GREEN,
                                 SYNC_PERIPHERY_HIT_BLUE,
                                 SYNC_PERIPHERY_HIT_FLASH_MS);
    }
}

//! \brief Отклик «промах»
void SyncPeriphery_Miss(void)
{
    for (uint8_t ledIndex = 0u; ledIndex < LEDS_QUANTITY; ledIndex++)
    {
        SyncPeriphery_StartFlash(ledIndex,
                                 SYNC_PERIPHERY_MISS_RED,
                                 SYNC_PERIPHERY_MISS_GREEN,
                                 SYNC_PERIPHERY_MISS_BLUE,
                                 SYNC_PERIPHERY_MISS_FLASH_MS);
    }
}
