#include <stdint.h>
#include <string.h>

#include "LedsDriver.h"
#include "LedsEffects.h"

#define MAX_COMPONENT_VALUE                 255u    //!< Максимальное значение цветовой компоненты
#define MAX_PERCENT                         100u    //!< Максимальное значение процента
#define HUE_CIRCLE                          256u    //!< Полный круг оттенков (0-255)
#define RAINBOW_SECTORS_QUANTITY            6u      //!< Количество секторов цветового круга
#define RAINBOW_SECTOR_SIZE                 (HUE_CIRCLE / RAINBOW_SECTORS_QUANTITY) //!< Размер сектора
#define RAINBOW_FRACTION_MAX                255u    //!< Максимум дроби внутри сектора
#define RAINBOW_HUE_STEP                    (HUE_CIRCLE / LEDS_QUANTITY) //!< Шаг оттенка по ленте

#define RAINBOW_SECTOR_RED_TO_YELLOW        0u      //!< Сектор: красный -> жёлтый
#define RAINBOW_SECTOR_YELLOW_TO_GREEN      1u      //!< Сектор: жёлтый -> зелёный
#define RAINBOW_SECTOR_GREEN_TO_CYAN        2u      //!< Сектор: зелёный -> голубой
#define RAINBOW_SECTOR_CYAN_TO_BLUE         3u      //!< Сектор: голубой -> синий
#define RAINBOW_SECTOR_BLUE_TO_MAGENTA      4u      //!< Сектор: синий -> пурпурный
#define RAINBOW_SECTOR_MAGENTA_TO_RED       5u      //!< Сектор: пурпурный -> красный

#define BATTERY_GREEN_MIN_PERCENT           50u     //!< Нижняя граница зелёного диапазона, %
#define BATTERY_YELLOW_MIN_PERCENT          20u     //!< Нижняя граница жёлтого диапазона, %
#define BATTERY_ROUND_UP_OFFSET             (MAX_PERCENT - 1u) //!< Смещение для округления вверх

#define BATTERY_GREEN_RED                   0u      //!< Красная компонента зелёного
#define BATTERY_GREEN_GREEN                 255u    //!< Зелёная компонента зелёного
#define BATTERY_GREEN_BLUE                  0u      //!< Синяя компонента зелёного

#define BATTERY_YELLOW_RED                  255u    //!< Красная компонента жёлтого
#define BATTERY_YELLOW_GREEN                255u    //!< Зелёная компонента жёлтого
#define BATTERY_YELLOW_BLUE                 0u      //!< Синяя компонента жёлтого

#define BATTERY_RED_RED                     255u    //!< Красная компонента красного
#define BATTERY_RED_GREEN                   0u      //!< Зелёная компонента красного
#define BATTERY_RED_BLUE                    0u      //!< Синяя компонента красного

#define KNIGHT_RIDER_POSITIONS              (2u * LEDS_QUANTITY - 2u) //!< Число позиций огонька (туда-обратно)
#define CHASE_TAIL_LENGTH                   3u      //!< Длина хвоста погони, LED

#define COLOR_CYCLE_PHASE_MAX               2u      //!< Количество полуфаз Color Cycle
#define COLOR_CYCLE_PHASE_HALF              1u      //!< Граница между полуфазами

#define LFSR_INITIAL_VALUE                  0xACu   //!< Начальное значение LFSR (ненулевое)
#define LFSR_POLYNOMIAL                     0xB8u   //!< Полином обратной связи LFSR (x^8+x^6+x^5+x^4+1)

#define BRIGHTNESS_RATIO_MAX                MAX_COMPONENT_VALUE //!< Максимум коэффициента яркости

//! \brief Состояние одного эффекта
typedef struct
{
    LedsEffectsParams params;       //!< Параметры эффекта
    uint32_t          elapsedMs;    //!< Прошедшее время, мс
    uint8_t           finished;     //!< Флаг завершения
} LedsEffect;

static LedsEffect currentEffect = { 0 };                            //!< Текущий активный эффект
static LedsEffect effectStack[LEDS_EFFECTS_STACK_SIZE] = { 0 };     //!< Стек вытесненных эффектов
static uint8_t    effectStackTop = 0;                               //!< Вершина стека (0 = пуст)
static uint8_t    initialized = 0;                                  //!< Флаг инициализации

static uint8_t    randomColors[LEDS_QUANTITY][3] = { 0 };           //!< Таблица случайных цветов (R, G, B)
static uint8_t    lfsrState = LFSR_INITIAL_VALUE;                   //!< Состояние LFSR

static uint8_t    sparkleCurrentLed = 0;                            //!< Текущий горящий LED Sparkle
static uint32_t   sparkleLastUpdateMs = 0;                          //!< Время последней вспышки Sparkle

//! \brief Получение следующего значения LFSR
//! \return Очередное псевдослучайное значение
static uint8_t LedsEffects_LfsrNext(void)
{
    lfsrState = (uint8_t)((lfsrState >> 1) ^ (-(lfsrState & 1u) & LFSR_POLYNOMIAL));

    return lfsrState;
}

//! \brief Генерация таблицы случайных цветов
static void LedsEffects_GenerateRandomColors(void)
{
    for (uint8_t ledIndex = 0u; ledIndex < LEDS_QUANTITY; ledIndex++)
    {
        randomColors[ledIndex][0] = LedsEffects_LfsrNext();
        randomColors[ledIndex][1] = LedsEffects_LfsrNext();
        randomColors[ledIndex][2] = LedsEffects_LfsrNext();
    }
}

//! \brief Преобразование hue в RGB по цветовому кругу
//! \param[in]  hue      Оттенок (0-255)
//! \param[out] pRed     Красная компонента
//! \param[out] pGreen   Зелёная компонента
//! \param[out] pBlue    Синяя компонента
static void LedsEffects_HueToRgb(uint8_t hue, uint8_t *pRed, uint8_t *pGreen, uint8_t *pBlue)
{
    uint8_t sector = hue / RAINBOW_SECTOR_SIZE;
    uint8_t position = hue % RAINBOW_SECTOR_SIZE;
    uint8_t fraction = (uint8_t)(((uint16_t)position * RAINBOW_FRACTION_MAX) / RAINBOW_SECTOR_SIZE);

    uint8_t red = 0u;
    uint8_t green = 0u;
    uint8_t blue = 0u;

    switch (sector)
    {
        case RAINBOW_SECTOR_RED_TO_YELLOW:
        {
            red = MAX_COMPONENT_VALUE;
            green = fraction;
            blue = 0u;

            break;
        }

        case RAINBOW_SECTOR_YELLOW_TO_GREEN:
        {
            red = MAX_COMPONENT_VALUE - fraction;
            green = MAX_COMPONENT_VALUE;
            blue = 0u;

            break;
        }

        case RAINBOW_SECTOR_GREEN_TO_CYAN:
        {
            red = 0u;
            green = MAX_COMPONENT_VALUE;
            blue = fraction;

            break;
        }

        case RAINBOW_SECTOR_CYAN_TO_BLUE:
        {
            red = 0u;
            green = MAX_COMPONENT_VALUE - fraction;
            blue = MAX_COMPONENT_VALUE;

            break;
        }

        case RAINBOW_SECTOR_BLUE_TO_MAGENTA:
        {
            red = fraction;
            green = 0u;
            blue = MAX_COMPONENT_VALUE;

            break;
        }

        case RAINBOW_SECTOR_MAGENTA_TO_RED:
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

    *pRed = red;
    *pGreen = green;
    *pBlue = blue;
}

//! \brief Масштабирование компоненты цвета на яркость
//! \param[in] component  Исходная компонента
//! \param[in] brightness Яркость (0-255)
//! \return Масштабированная компонента
static uint8_t LedsEffects_ScaleByBrightness(uint8_t component, uint8_t brightness)
{
    return (uint8_t)(((uint16_t)component * brightness) / BRIGHTNESS_RATIO_MAX);
}

//! \brief Интерполяция между двумя компонентами
//! \param[in] from  Начальное значение
//! \param[in] to    Конечное значение
//! \param[in] ratio Коэффициент (0-255)
//! \return Интерполированное значение
static uint8_t LedsEffects_Lerp(uint8_t from, uint8_t to, uint8_t ratio)
{
    if (to >= from)
    {
        uint16_t delta = (uint16_t)(to - from);

        return (uint8_t)(from + (delta * ratio) / BRIGHTNESS_RATIO_MAX);
    }

    uint16_t delta = (uint16_t)(from - to);

    return (uint8_t)(from - (delta * ratio) / BRIGHTNESS_RATIO_MAX);
}

//! \brief Вычисление коэффициента CYCLE по времени (0..255 туда-обратно)
//! \param[in] pParams   Параметры эффекта
//! \param[in] elapsedMs Прошедшее время
//! \return Коэффициент (0-255)
static uint8_t LedsEffects_GetCycleRatio(const LedsEffectsParams *pParams, uint32_t elapsedMs)
{
    uint32_t periodMs = pParams->periodMs;

    if (0u == periodMs)
    {
        return 0u;
    }

    uint32_t halfPeriod = periodMs / 2u;

    if (0u == halfPeriod)
    {
        halfPeriod = 1u;
    }

    uint32_t phase = elapsedMs % periodMs;
    uint32_t ratio;

    if (phase < halfPeriod)
    {
        ratio = (phase * BRIGHTNESS_RATIO_MAX) / halfPeriod;
    }
    else
    {
        ratio = ((periodMs - phase) * BRIGHTNESS_RATIO_MAX) / halfPeriod;
    }

    if (ratio > BRIGHTNESS_RATIO_MAX)
    {
        ratio = BRIGHTNESS_RATIO_MAX;
    }

    return (uint8_t)ratio;
}

//! \brief Применение цвета к одному светодиоду
//! \param[in] ledIndex   Индекс светодиода
//! \param[in] pParams    Параметры эффекта
//! \param[in] brightness Яркость (0-255)
//! \param[in] elapsedMs  Прошедшее время, мс (для CYCLE)
static void LedsEffects_ApplyColor(uint8_t ledIndex,
                                   const LedsEffectsParams *pParams,
                                   uint8_t brightness,
                                   uint32_t elapsedMs)
{
    if (ledIndex >= LEDS_QUANTITY)
    {
        return;
    }

    uint8_t red = 0u;
    uint8_t green = 0u;
    uint8_t blue = 0u;

    switch (pParams->colorMode)
    {
        case LEDS_EFFECTS_COLOR_SOLID:
        {
            red = pParams->red1;
            green = pParams->green1;
            blue = pParams->blue1;

            break;
        }

        case LEDS_EFFECTS_COLOR_RAINBOW:
        {
            uint8_t hue = (uint8_t)((uint16_t)ledIndex * RAINBOW_HUE_STEP);

            LedsEffects_HueToRgb(hue, &red, &green, &blue);

            break;
        }

        case LEDS_EFFECTS_COLOR_GRADIENT:
        {
            uint8_t ratio = (uint8_t)(((uint16_t)ledIndex * BRIGHTNESS_RATIO_MAX) /
                                      (LEDS_QUANTITY - 1u));

            red = LedsEffects_Lerp(pParams->red1, pParams->red2, ratio);
            green = LedsEffects_Lerp(pParams->green1, pParams->green2, ratio);
            blue = LedsEffects_Lerp(pParams->blue1, pParams->blue2, ratio);

            break;
        }

        case LEDS_EFFECTS_COLOR_MODE_CYCLE:
        {
            uint8_t ratio = LedsEffects_GetCycleRatio(pParams, elapsedMs);

            red = LedsEffects_Lerp(pParams->red1, pParams->red2, ratio);
            green = LedsEffects_Lerp(pParams->green1, pParams->green2, ratio);
            blue = LedsEffects_Lerp(pParams->blue1, pParams->blue2, ratio);

            break;
        }

        case LEDS_EFFECTS_COLOR_RANDOM:
        {
            red = randomColors[ledIndex][0];
            green = randomColors[ledIndex][1];
            blue = randomColors[ledIndex][2];

            break;
        }

        default:
        {
            break;
        }
    }

    red = LedsEffects_ScaleByBrightness(red, brightness);
    green = LedsEffects_ScaleByBrightness(green, brightness);
    blue = LedsEffects_ScaleByBrightness(blue, brightness);

    LedsDriver_SetColor(ledIndex, red, green, blue);
}

//! \brief Применение цвета ко всей ленте
//! \param[in] pParams    Параметры эффекта
//! \param[in] brightness Яркость (0-255)
//! \param[in] elapsedMs  Прошедшее время, мс (для CYCLE)
static void LedsEffects_ApplyAll(const LedsEffectsParams *pParams,
                                 uint8_t brightness,
                                 uint32_t elapsedMs)
{
    for (uint8_t ledIndex = 0u; ledIndex < LEDS_QUANTITY; ledIndex++)
    {
        LedsEffects_ApplyColor(ledIndex, pParams, brightness, elapsedMs);
    }
}

//! \brief Проверка корректности параметров эффекта
//! \param[in] pParams - параметры эффекта
//! \return 1 - параметры корректны, 0 - некорректны
static uint8_t LedsEffects_AreParamsValid(const LedsEffectsParams *pParams)
{
    if (NULL == pParams)
    {
        return 0u;
    }

    if (pParams->type <= LEDS_EFFECTS_NONE)
    {
        return 0u;
    }

    if (pParams->type > LEDS_EFFECTS_THEATER_CHASE)
    {
        return 0u;
    }

    if (pParams->priority > LEDS_EFFECTS_PRIORITY_HIGH)
    {
        return 0u;
    }

    if (pParams->colorMode > LEDS_EFFECTS_COLOR_RANDOM)
    {
        return 0u;
    }

    return 1u;
}

//! \brief Нормализация параметров эффекта
//! \param[in,out] pParams - параметры эффекта
static void LedsEffects_NormalizeParams(LedsEffectsParams *pParams)
{
    if (pParams->durationMs > LEDS_EFFECTS_MAX_DURATION_MS)
    {
        pParams->durationMs = LEDS_EFFECTS_MAX_DURATION_MS;
    }

    if (0u == pParams->periodMs)
    {
        switch (pParams->type)
        {
            case LEDS_EFFECTS_BLINK:
            {
                pParams->periodMs = LEDS_EFFECTS_DEFAULT_BLINK_PERIOD_MS;

                break;
            }

            case LEDS_EFFECTS_PULSE:
            {
                pParams->periodMs = LEDS_EFFECTS_DEFAULT_PULSE_PERIOD_MS;

                break;
            }

            case LEDS_EFFECTS_RAINBOW:
            {
                pParams->periodMs = LEDS_EFFECTS_DEFAULT_RAINBOW_PERIOD_MS;

                break;
            }

            case LEDS_EFFECTS_KNIGHT_RIDER:
            {
                pParams->periodMs = LEDS_EFFECTS_DEFAULT_KNIGHT_RIDER_PERIOD_MS;

                break;
            }

            case LEDS_EFFECTS_COLOR_CYCLE:
            {
                pParams->periodMs = LEDS_EFFECTS_DEFAULT_COLOR_CYCLE_PERIOD_MS;

                break;
            }

            case LEDS_EFFECTS_CHASE:
            {
                pParams->periodMs = LEDS_EFFECTS_DEFAULT_CHASE_PERIOD_MS;

                break;
            }

            case LEDS_EFFECTS_SPARKLE:
            {
                pParams->periodMs = LEDS_EFFECTS_DEFAULT_SPARKLE_PERIOD_MS;

                break;
            }

            case LEDS_EFFECTS_THEATER_CHASE:
            {
                pParams->periodMs = LEDS_EFFECTS_DEFAULT_THEATER_CHASE_PERIOD_MS;

                break;
            }

            default:
            {
                break;
            }
        }
    }
}

//! \brief Вытеснение самого старого низкоприоритетного эффекта из стека
//! \details Если низкоприоритетных нет — вытесняется самый старый (нижний).
static void LedsEffects_EvictOldestFromStack(void)
{
    if (0u == effectStackTop)
    {
        return;
    }

    uint8_t evictIndex = 0u;
    uint8_t foundLow = 0u;

    for (uint8_t index = 0u; index < effectStackTop; index++)
    {
        if (LEDS_EFFECTS_PRIORITY_LOW == effectStack[index].params.priority)
        {
            evictIndex = index;
            foundLow = 1u;

            break;
        }
    }

    if (0u == foundLow)
    {
        evictIndex = 0u;
    }

    for (uint8_t index = evictIndex; index < (effectStackTop - 1u); index++)
    {
        effectStack[index] = effectStack[index + 1u];
    }

    effectStackTop--;
}

//! \brief Сохранение текущего эффекта в стек
static void LedsEffects_PushCurrentToStack(void)
{
    if (effectStackTop >= LEDS_EFFECTS_STACK_SIZE)
    {
        LedsEffects_EvictOldestFromStack();
    }

    effectStack[effectStackTop] = currentEffect;
    effectStackTop++;
}

//! \brief Извлечение эффекта из стека в текущий
//! \return 1 - эффект извлечён, 0 - стек пуст
static uint8_t LedsEffects_PopStackToCurrent(void)
{
    if (0u == effectStackTop)
    {
        return 0u;
    }

    effectStackTop--;
    currentEffect = effectStack[effectStackTop];

    if (LEDS_EFFECTS_SPARKLE == currentEffect.params.type)
    {
        sparkleCurrentLed = 0u;
        sparkleLastUpdateMs = currentEffect.elapsedMs;
    }

    return 1u;
}

//! \brief Сброс текущего эффекта
static void LedsEffects_ResetCurrent(void)
{
    memset(&currentEffect, 0, sizeof(currentEffect));
    currentEffect.params.type = LEDS_EFFECTS_NONE;
}

//! \brief Проверка завершения эффекта по времени и/или количеству
//! \param[in] pEffect - состояние эффекта
//! \return 1 - эффект завершён, 0 - активен
static uint8_t LedsEffects_IsEffectFinished(const LedsEffect *pEffect)
{
    uint16_t durationMs = pEffect->params.durationMs;
    uint8_t  count = pEffect->params.count;

    if ((LEDS_EFFECTS_BLINK == pEffect->params.type) ||
        (LEDS_EFFECTS_PULSE == pEffect->params.type))
    {
        if ((0u != durationMs) && (pEffect->elapsedMs >= durationMs))
        {
            return 1u;
        }

        if (0u != count)
        {
            uint32_t periodMs = pEffect->params.periodMs;

            if (0u != periodMs)
            {
                uint32_t completedCycles = pEffect->elapsedMs / periodMs;

                if (completedCycles >= count)
                {
                    return 1u;
                }
            }
        }

        return 0u;
    }

    if (LEDS_EFFECTS_RAINBOW == pEffect->params.type)
    {
        if ((0u != durationMs) && (pEffect->elapsedMs >= durationMs))
        {
            return 1u;
        }

        return 0u;
    }

    if ((LEDS_EFFECTS_FADE_IN == pEffect->params.type) ||
        (LEDS_EFFECTS_FADE_OUT == pEffect->params.type) ||
        (LEDS_EFFECTS_PROGRESS == pEffect->params.type))
    {
        if (0u == durationMs)
        {
            return 0u;
        }

        if (pEffect->elapsedMs >= durationMs)
        {
            return 1u;
        }

        return 0u;
    }

    if ((0u != durationMs) && (pEffect->elapsedMs >= durationMs))
    {
        return 1u;
    }

    if (0u != count)
    {
        uint32_t periodMs = pEffect->params.periodMs;

        if (0u != periodMs)
        {
            uint32_t completedCycles = pEffect->elapsedMs / periodMs;

            if (completedCycles >= count)
            {
                return 1u;
            }
        }
    }

    return 0u;
}

//! \brief Применение шага эффекта Fade In
//! \param[in] pEffect - состояние эффекта
static void LedsEffects_ApplyFadeIn(const LedsEffect *pEffect)
{
    uint32_t durationMs = pEffect->params.durationMs;

    if (0u == durationMs)
    {
        LedsEffects_ApplyAll(&pEffect->params, MAX_COMPONENT_VALUE, pEffect->elapsedMs);

        return;
    }

    uint32_t elapsed = pEffect->elapsedMs;

    if (elapsed > durationMs)
    {
        elapsed = durationMs;
    }

    uint8_t brightness = (uint8_t)((elapsed * MAX_COMPONENT_VALUE) / durationMs);

    LedsEffects_ApplyAll(&pEffect->params, brightness, pEffect->elapsedMs);
}

//! \brief Применение шага эффекта Fade Out
//! \param[in] pEffect - состояние эффекта
static void LedsEffects_ApplyFadeOut(const LedsEffect *pEffect)
{
    uint32_t durationMs = pEffect->params.durationMs;

    if (0u == durationMs)
    {
        LedsEffects_ApplyAll(&pEffect->params, 0u, pEffect->elapsedMs);

        return;
    }

    uint32_t elapsed = pEffect->elapsedMs;

    if (elapsed > durationMs)
    {
        elapsed = durationMs;
    }

    uint32_t remaining = durationMs - elapsed;

    uint8_t brightness = (uint8_t)((remaining * MAX_COMPONENT_VALUE) / durationMs);

    LedsEffects_ApplyAll(&pEffect->params, brightness, pEffect->elapsedMs);
}

//! \brief Применение шага эффекта Blink
//! \param[in] pEffect - состояние эффекта
static void LedsEffects_ApplyBlink(const LedsEffect *pEffect)
{
    uint32_t periodMs = pEffect->params.periodMs;

    if (0u == periodMs)
    {
        LedsEffects_ApplyAll(&pEffect->params, 0u, pEffect->elapsedMs);

        return;
    }

    uint32_t halfPeriod = periodMs / 2u;

    if (0u == halfPeriod)
    {
        halfPeriod = 1u;
    }

    uint32_t phase = pEffect->elapsedMs % periodMs;
    uint8_t brightness = (phase < halfPeriod) ? MAX_COMPONENT_VALUE : 0u;

    LedsEffects_ApplyAll(&pEffect->params, brightness, pEffect->elapsedMs);
}

//! \brief Применение шага эффекта Pulse
//! \param[in] pEffect - состояние эффекта
static void LedsEffects_ApplyPulse(const LedsEffect *pEffect)
{
    uint32_t periodMs = pEffect->params.periodMs;

    if (0u == periodMs)
    {
        LedsEffects_ApplyAll(&pEffect->params, 0u, pEffect->elapsedMs);

        return;
    }

    uint32_t halfPeriod = periodMs / 2u;

    if (0u == halfPeriod)
    {
        halfPeriod = 1u;
    }

    uint32_t phase = pEffect->elapsedMs % periodMs;
    uint32_t brightness;

    if (phase < halfPeriod)
    {
        brightness = (phase * MAX_COMPONENT_VALUE) / halfPeriod;
    }
    else
    {
        brightness = ((periodMs - phase) * MAX_COMPONENT_VALUE) / halfPeriod;
    }

    LedsEffects_ApplyAll(&pEffect->params, (uint8_t)brightness, pEffect->elapsedMs);
}

//! \brief Применение шага эффекта Rainbow (анимированная, colorMode игнорируется)
//! \param[in] pEffect - состояние эффекта
static void LedsEffects_ApplyRainbow(const LedsEffect *pEffect)
{
    uint32_t periodMs = pEffect->params.periodMs;

    if (0u == periodMs)
    {
        return;
    }

    uint8_t offset = (uint8_t)((pEffect->elapsedMs * HUE_CIRCLE) / periodMs);

    for (uint8_t ledIndex = 0u; ledIndex < LEDS_QUANTITY; ledIndex++)
    {
        uint8_t hue = (uint8_t)((uint16_t)offset + (uint16_t)ledIndex * RAINBOW_HUE_STEP);

        uint8_t red = 0u;
        uint8_t green = 0u;
        uint8_t blue = 0u;

        LedsEffects_HueToRgb(hue, &red, &green, &blue);

        LedsDriver_SetColor(ledIndex, red, green, blue);
    }
}

//! \brief Применение шага эффекта Progress
//! \param[in] pEffect - состояние эффекта
static void LedsEffects_ApplyProgress(const LedsEffect *pEffect)
{
    uint32_t durationMs = pEffect->params.durationMs;

    uint8_t ledsLit = 0u;

    if (0u != durationMs)
    {
        uint32_t elapsed = pEffect->elapsedMs;

        if (elapsed > durationMs)
        {
            elapsed = durationMs;
        }

        ledsLit = (uint8_t)(((elapsed * LEDS_QUANTITY) + durationMs - 1u) / durationMs);

        if (ledsLit > LEDS_QUANTITY)
        {
            ledsLit = LEDS_QUANTITY;
        }
    }
    else
    {
        ledsLit = LEDS_QUANTITY;
    }

    for (uint8_t ledIndex = 0u; ledIndex < LEDS_QUANTITY; ledIndex++)
    {
        uint8_t brightness = (ledIndex < ledsLit) ? MAX_COMPONENT_VALUE : 0u;

        LedsEffects_ApplyColor(ledIndex, &pEffect->params, brightness, pEffect->elapsedMs);
    }
}

//! \brief Применение шага эффекта Knight Rider
//! \param[in] pEffect - состояние эффекта
static void LedsEffects_ApplyKnightRider(const LedsEffect *pEffect)
{
    uint32_t periodMs = pEffect->params.periodMs;

    if (0u == periodMs)
    {
        return;
    }

    uint32_t stepMs = periodMs / KNIGHT_RIDER_POSITIONS;

    if (0u == stepMs)
    {
        stepMs = 1u;
    }

    uint32_t stepIndex = pEffect->elapsedMs / stepMs;
    uint32_t position = stepIndex % KNIGHT_RIDER_POSITIONS;

    if (position >= LEDS_QUANTITY)
    {
        position = KNIGHT_RIDER_POSITIONS - position;
    }

    for (uint8_t ledIndex = 0u; ledIndex < LEDS_QUANTITY; ledIndex++)
    {
        uint8_t brightness = (ledIndex == (uint8_t)position) ? MAX_COMPONENT_VALUE : 0u;

        LedsEffects_ApplyColor(ledIndex, &pEffect->params, brightness, pEffect->elapsedMs);
    }
}

//! \brief Применение шага эффекта Color Cycle (colorMode игнорируется)
//! \param[in] pEffect - состояние эффекта
static void LedsEffects_ApplyColorCycle(const LedsEffect *pEffect)
{
    uint8_t ratio = LedsEffects_GetCycleRatio(&pEffect->params, pEffect->elapsedMs);

    uint8_t red = LedsEffects_Lerp(pEffect->params.red1, pEffect->params.red2, ratio);
    uint8_t green = LedsEffects_Lerp(pEffect->params.green1, pEffect->params.green2, ratio);
    uint8_t blue = LedsEffects_Lerp(pEffect->params.blue1, pEffect->params.blue2, ratio);

    LedsDriver_SetAll(red, green, blue);
}

//! \brief Применение шага эффекта Chase
//! \param[in] pEffect - состояние эффекта
static void LedsEffects_ApplyChase(const LedsEffect *pEffect)
{
    uint32_t periodMs = pEffect->params.periodMs;

    if (0u == periodMs)
    {
        return;
    }

    uint32_t stepMs = periodMs / LEDS_QUANTITY;

    if (0u == stepMs)
    {
        stepMs = 1u;
    }

    uint32_t headPosition = (pEffect->elapsedMs / stepMs) % LEDS_QUANTITY;

    for (uint8_t ledIndex = 0u; ledIndex < LEDS_QUANTITY; ledIndex++)
    {
        uint8_t distance = (uint8_t)((headPosition - ledIndex + LEDS_QUANTITY) % LEDS_QUANTITY);
        uint8_t brightness = 0u;

        if (distance < CHASE_TAIL_LENGTH)
        {
            brightness = (uint8_t)(((CHASE_TAIL_LENGTH - distance) * MAX_COMPONENT_VALUE) /
                                   CHASE_TAIL_LENGTH);
        }

        LedsEffects_ApplyColor(ledIndex, &pEffect->params, brightness, pEffect->elapsedMs);
    }
}

//! \brief Применение шага эффекта Sparkle
//! \param[in] pEffect - состояние эффекта
static void LedsEffects_ApplySparkle(const LedsEffect *pEffect)
{
    uint32_t periodMs = pEffect->params.periodMs;

    if (0u == periodMs)
    {
        return;
    }

    uint32_t stepMs = periodMs / LEDS_QUANTITY;

    if (0u == stepMs)
    {
        stepMs = 1u;
    }

    if ((pEffect->elapsedMs - sparkleLastUpdateMs) >= stepMs)
    {
        sparkleCurrentLed = (uint8_t)(LedsEffects_LfsrNext() % LEDS_QUANTITY);
        sparkleLastUpdateMs = pEffect->elapsedMs;
    }

    for (uint8_t ledIndex = 0u; ledIndex < LEDS_QUANTITY; ledIndex++)
    {
        uint8_t brightness = (ledIndex == sparkleCurrentLed) ? MAX_COMPONENT_VALUE : 0u;

        LedsEffects_ApplyColor(ledIndex, &pEffect->params, brightness, pEffect->elapsedMs);
    }
}

//! \brief Применение шага эффекта Theater Chase
//! \param[in] pEffect - состояние эффекта
static void LedsEffects_ApplyTheaterChase(const LedsEffect *pEffect)
{
    uint32_t periodMs = pEffect->params.periodMs;

    if (0u == periodMs)
    {
        LedsEffects_ApplyAll(&pEffect->params, 0u, pEffect->elapsedMs);

        return;
    }

    uint32_t halfPeriod = periodMs / 2u;

    if (0u == halfPeriod)
    {
        halfPeriod = 1u;
    }

    uint32_t phase = (pEffect->elapsedMs / halfPeriod) % 2u;

    for (uint8_t ledIndex = 0u; ledIndex < LEDS_QUANTITY; ledIndex++)
    {
        uint8_t brightness = 0u;

        if ((ledIndex % 2u) == (uint8_t)phase)
        {
            brightness = MAX_COMPONENT_VALUE;
        }

        LedsEffects_ApplyColor(ledIndex, &pEffect->params, brightness, pEffect->elapsedMs);
    }
}

//! \brief Применение текущего эффекта (шаг обновления)
static void LedsEffects_ApplyCurrent(void)
{
    switch (currentEffect.params.type)
    {
        case LEDS_EFFECTS_FADE_IN:
        {
            LedsEffects_ApplyFadeIn(&currentEffect);

            break;
        }

        case LEDS_EFFECTS_FADE_OUT:
        {
            LedsEffects_ApplyFadeOut(&currentEffect);

            break;
        }

        case LEDS_EFFECTS_BLINK:
        {
            LedsEffects_ApplyBlink(&currentEffect);

            break;
        }

        case LEDS_EFFECTS_PULSE:
        {
            LedsEffects_ApplyPulse(&currentEffect);

            break;
        }

        case LEDS_EFFECTS_RAINBOW:
        {
            LedsEffects_ApplyRainbow(&currentEffect);

            break;
        }

        case LEDS_EFFECTS_PROGRESS:
        {
            LedsEffects_ApplyProgress(&currentEffect);

            break;
        }

        case LEDS_EFFECTS_KNIGHT_RIDER:
        {
            LedsEffects_ApplyKnightRider(&currentEffect);

            break;
        }

        case LEDS_EFFECTS_COLOR_CYCLE:
        {
            LedsEffects_ApplyColorCycle(&currentEffect);

            break;
        }

        case LEDS_EFFECTS_CHASE:
        {
            LedsEffects_ApplyChase(&currentEffect);

            break;
        }

        case LEDS_EFFECTS_SPARKLE:
        {
            LedsEffects_ApplySparkle(&currentEffect);

            break;
        }

        case LEDS_EFFECTS_THEATER_CHASE:
        {
            LedsEffects_ApplyTheaterChase(&currentEffect);

            break;
        }

        default:
        {
            break;
        }
    }
}

//! \brief Инициализация модуля световых эффектов
void LedsEffects_Init(void)
{
    if (0u != initialized)
    {
        return;
    }

    LedsEffects_ResetCurrent();
    memset(effectStack, 0, sizeof(effectStack));
    effectStackTop = 0u;

    lfsrState = LFSR_INITIAL_VALUE;
    LedsEffects_GenerateRandomColors();

    sparkleCurrentLed = 0u;
    sparkleLastUpdateMs = 0u;

    initialized = 1u;
}

//! \brief Запуск светового эффекта
void LedsEffects_Start(const LedsEffectsParams *pParams)
{
    if (0u == initialized)
    {
        return;
    }

    if (0u == LedsEffects_AreParamsValid(pParams))
    {
        return;
    }

    LedsEffectsParams normalizedParams = *pParams;
    LedsEffects_NormalizeParams(&normalizedParams);

    LedsEffectsPriority currentPriority = LEDS_EFFECTS_PRIORITY_LOW;

    if (LEDS_EFFECTS_NONE != currentEffect.params.type)
    {
        currentPriority = currentEffect.params.priority;
    }

    if (normalizedParams.priority < currentPriority)
    {
        return;
    }

    if (normalizedParams.priority > currentPriority)
    {
        if (LEDS_EFFECTS_NONE != currentEffect.params.type)
        {
            LedsEffects_PushCurrentToStack();
        }
    }

    memset(&currentEffect, 0, sizeof(currentEffect));
    currentEffect.params = normalizedParams;
    currentEffect.elapsedMs = 0u;
    currentEffect.finished = 0u;

    if (LEDS_EFFECTS_SPARKLE == normalizedParams.type)
    {
        sparkleCurrentLed = 0u;
        sparkleLastUpdateMs = 0u;
    }
}

//! \brief Обновление текущего эффекта
void LedsEffects_Update(void)
{
    if (0u == initialized)
    {
        return;
    }

    if (LEDS_EFFECTS_NONE == currentEffect.params.type)
    {
        return;
    }

    if (0u != currentEffect.finished)
    {
        return;
    }

    currentEffect.elapsedMs += (uint32_t)LEDS_EFFECTS_UPDATE_PERIOD_MS;

    if (0u != LedsEffects_IsEffectFinished(&currentEffect))
    {
        currentEffect.finished = 1u;

        if (0u != LedsEffects_PopStackToCurrent())
        {
            LedsEffects_ApplyCurrent();
        }

        return;
    }

    LedsEffects_ApplyCurrent();
}

//! \brief Остановка текущего эффекта
void LedsEffects_Stop(void)
{
    if (0u == initialized)
    {
        return;
    }

    LedsEffects_ResetCurrent();
}

//! \brief Остановка всех эффектов, включая стек
void LedsEffects_StopAll(void)
{
    if (0u == initialized)
    {
        return;
    }

    LedsEffects_ResetCurrent();
    memset(effectStack, 0, sizeof(effectStack));
    effectStackTop = 0u;
}

//! \brief Проверка завершения текущего эффекта
uint8_t LedsEffects_IsFinished(void)
{
    if (LEDS_EFFECTS_NONE == currentEffect.params.type)
    {
        return 1u;
    }

    return currentEffect.finished;
}

//! \brief Получение приоритета текущего эффекта
LedsEffectsPriority LedsEffects_GetPriority(void)
{
    if (LEDS_EFFECTS_NONE == currentEffect.params.type)
    {
        return LEDS_EFFECTS_PRIORITY_LOW;
    }

    return currentEffect.params.priority;
}

//! \brief Получение типа текущего эффекта
LedsEffectsType LedsEffects_GetType(void)
{
    return currentEffect.params.type;
}

//! \brief Статичная индикация заряда батареи
void LedsEffects_ShowBattery(uint8_t percent)
{
    if (0u == initialized)
    {
        return;
    }

    if (percent > MAX_PERCENT)
    {
        percent = MAX_PERCENT;
    }

    uint8_t ledsLit = 0u;

    if (percent > 0u)
    {
        ledsLit = (uint8_t)(((uint16_t)percent * LEDS_QUANTITY + BATTERY_ROUND_UP_OFFSET) / MAX_PERCENT);
    }

    uint8_t red = 0u;
    uint8_t green = 0u;
    uint8_t blue = 0u;

    if (percent >= BATTERY_GREEN_MIN_PERCENT)
    {
        red = BATTERY_GREEN_RED;
        green = BATTERY_GREEN_GREEN;
        blue = BATTERY_GREEN_BLUE;
    }
    else if (percent >= BATTERY_YELLOW_MIN_PERCENT)
    {
        red = BATTERY_YELLOW_RED;
        green = BATTERY_YELLOW_GREEN;
        blue = BATTERY_YELLOW_BLUE;
    }
    else
    {
        red = BATTERY_RED_RED;
        green = BATTERY_RED_GREEN;
        blue = BATTERY_RED_BLUE;
    }

    for (uint8_t ledIndex = 0u; ledIndex < LEDS_QUANTITY; ledIndex++)
    {
        if (ledIndex < ledsLit)
        {
            LedsDriver_SetColor(ledIndex, red, green, blue);
        }
        else
        {
            LedsDriver_SetColor(ledIndex, 0u, 0u, 0u);
        }
    }
}
