#include <stdint.h>
#include <string.h>

#include "stm32f1xx_hal.h"

#include "BuzzersDriver.h"
#include "Hardware.h"
#include "UserTimer.h"

//! Период обработки тиков зуммеров, мс (точность длительностей ±1 мс, N-07)
#define BUZZER_TICK_PERIOD_MS            1u

//! Период обработки тиков зуммеров в тиках UserTimer (100 мкс)
#define BUZZER_TICK_PERIOD_TICKS         (USER_TIMER_PERIODS_IN_1_MS * BUZZER_TICK_PERIOD_MS)

//! Частота счёта таймера (после предделителя), Гц
#define BUZZER_COUNTER_CLOCK_HZ          BUZZER_TIMER_TICK_HZ

//! Максимальное значение громкости, %
#define BUZZER_MAX_VOLUME_PERCENT        100u

//! Коэффициент перевода громкости в duty (100 % громкости = 50 % duty)
#define BUZZER_VOLUME_TO_DUTY_DIVISOR    (BUZZER_MAX_VOLUME_PERCENT * 2u)

//! Значение CCR для тишины
#define BUZZER_CCR_SILENCE               0u

//! \brief Контекст одного зуммера
typedef struct
{
    uint8_t           sleeping;              //!< Флаг режима пониженного потребления
    uint8_t           playing;               //!< Флаг занятости (тон или мелодия)
    uint8_t           melodyActive;          //!< Флаг активной мелодии
    uint8_t           volumePercent;         //!< Громкость, 0–100 %
    const BuzzerNote *pNotes;                //!< Указатель на массив нот
    uint16_t          notesCount;            //!< Количество нот в мелодии
    uint16_t          currentNoteIndex;      //!< Индекс текущей ноты
    uint32_t          remainingMs;           //!< Остаток текущей ноты, мс
    void              (*pMelodyCompleteCallback)(void);                //!< Callback завершения мелодии
    void              (*pNoteStartCallback)(uint16_t, uint16_t);       //!< Callback начала ноты (noteIndex, frequencyHz)
} BuzzerContext;

//! Таблица частот нот (две октавы C4–B5 + пауза)
static const uint16_t buzzerNoteFrequencies[BUZZER_NOTES_COUNT] =
{
    262u,   //!< C4
    277u,   //!< C#4
    294u,   //!< D4
    311u,   //!< D#4
    330u,   //!< E4
    349u,   //!< F4
    370u,   //!< F#4
    392u,   //!< G4
    415u,   //!< G#4
    440u,   //!< A4
    466u,   //!< A#4
    494u,   //!< B4

    523u,   //!< C5
    554u,   //!< C#5
    587u,   //!< D5
    622u,   //!< D#5
    659u,   //!< E5
    698u,   //!< F5
    740u,   //!< F#5
    784u,   //!< G5
    831u,   //!< G#5
    880u,   //!< A5
    932u,   //!< A#5
    988u,   //!< B5

    0u      //!< REST
};

//! Контексты зуммеров
static BuzzerContext buzzerContexts[BUZZERS_QUANTITY] = { 0 };

//! Хэндл таймера зуммеров
extern TIM_HandleTypeDef TIM_BUZZERS_HTIM;

//! \brief Проверка корректности идентификатора зуммера
//! \param[in] buzzer Идентификатор зуммера
//! \return 1, если идентификатор корректен, иначе 0
static uint8_t BuzzersDriver_IsValidBuzzer(Buzzer buzzer)
{
    return (buzzer < BUZZERS_COUNT) ? 1u : 0u;
}

//! \brief Получить канал таймера для зуммера
//! \param[in] buzzer Идентификатор зуммера
//! \return Канал таймера
static uint32_t BuzzersDriver_GetChannel(Buzzer buzzer)
{
    if (BUZZER_LARGE == buzzer)
    {
        return LARGE_BUZZER_PWM_CHANNEL;
    }

    return SMALL_BUZZER_PWM_CHANNEL;
}

//! \brief Вычислить ARR для заданной частоты
//! \param[in] frequencyHz Частота, Гц
//! \return Значение ARR (0, если частота некорректна)
static uint16_t BuzzersDriver_CalculateArr(uint16_t frequencyHz)
{
    if ((frequencyHz < BUZZER_MIN_FREQUENCY_HZ) ||
        (frequencyHz > BUZZER_MAX_FREQUENCY_HZ))
    {
        return 0u;
    }

    uint32_t arr = (BUZZER_COUNTER_CLOCK_HZ / frequencyHz);

    if (arr == 0u)
    {
        return 0u;
    }

    arr = arr - 1u;

    if (arr > 0xFFFFu)
    {
        arr = 0xFFFFu;
    }

    return (uint16_t)arr;
}

//! \brief Вычислить CCR для заданного ARR и громкости
//! \param[in] arr Значение ARR
//! \param[in] volumePercent Громкость, 0–100 %
//! \return Значение CCR
static uint16_t BuzzersDriver_CalculateCcr(uint16_t arr, uint8_t volumePercent)
{
    if (volumePercent > BUZZER_MAX_VOLUME_PERCENT)
    {
        volumePercent = BUZZER_MAX_VOLUME_PERCENT;
    }

    uint32_t ccr = ((uint32_t)(arr + 1u) * volumePercent) / BUZZER_VOLUME_TO_DUTY_DIVISOR;

    if (ccr > 0xFFFFu)
    {
        ccr = 0xFFFFu;
    }

    return (uint16_t)ccr;
}

//! \brief Установить ARR таймера (общий для обоих каналов)
//! \param[in] arr Значение ARR
static void BuzzersDriver_SetArr(uint16_t arr)
{
    __HAL_TIM_SET_AUTORELOAD(&TIM_BUZZERS_HTIM, arr);
}

//! \brief Установить CCR для канала
//! \param[in] buzzer Идентификатор зуммера
//! \param[in] ccr Значение CCR
static void BuzzersDriver_SetCcr(Buzzer buzzer, uint16_t ccr)
{
    __HAL_TIM_SET_COMPARE(&TIM_BUZZERS_HTIM, BuzzersDriver_GetChannel(buzzer), ccr);
}

//! \brief Запустить генерацию на канале (если ещё не запущена)
//! \param[in] buzzer Идентификатор зуммера
static void BuzzersDriver_StartChannel(Buzzer buzzer)
{
    HAL_TIM_PWM_Start(&TIM_BUZZERS_HTIM, BuzzersDriver_GetChannel(buzzer));
}

//! \brief Применить частоту к таймеру (общий ARR) и обновить CCR канала
//! \param[in] buzzer Идентификатор зуммера
//! \param[in] frequencyHz Частота, Гц
static void BuzzersDriver_ApplyFrequency(Buzzer buzzer, uint16_t frequencyHz)
{
    uint16_t arr = BuzzersDriver_CalculateArr(frequencyHz);

    if (0u == arr)
    {
        BuzzersDriver_SetCcr(buzzer, BUZZER_CCR_SILENCE);

        return;
    }

    BuzzersDriver_SetArr(arr);

    uint16_t ccr = BuzzersDriver_CalculateCcr(arr, buzzerContexts[buzzer].volumePercent);

    BuzzersDriver_SetCcr(buzzer, ccr);
}

//! \brief Сбросить состояние мелодии
//! \param[in] buzzer Идентификатор зуммера
static void BuzzersDriver_ResetMelody(Buzzer buzzer)
{
    buzzerContexts[buzzer].melodyActive = 0u;
    buzzerContexts[buzzer].pNotes = NULL;
    buzzerContexts[buzzer].notesCount = 0u;
    buzzerContexts[buzzer].currentNoteIndex = 0u;
    buzzerContexts[buzzer].remainingMs = 0u;
}

//! \brief Проверить, что текущая нота мелодии — пауза
//! \param[in] buzzer Идентификатор зуммера
//! \return 1, если мелодия активна и текущая нота — пауза, иначе 0
static uint8_t BuzzersDriver_IsCurrentNoteRest(Buzzer buzzer)
{
    const BuzzerContext *pContext = &buzzerContexts[buzzer];

    if (0u == pContext->melodyActive)
    {
        return 0u;
    }

    const BuzzerNote *pNote = &pContext->pNotes[pContext->currentNoteIndex];

    return (0u == pNote->frequencyHz) ? 1u : 0u;
}

//! \brief Применить ноту мелодии и уведомить callback
//! \details Устанавливает ARR/CCR под ноту, затем (только для нот,
//!          не для пауз) вызывает pNoteStartCallback.
//! \param[in] buzzer Идентификатор зуммера
static void BuzzersDriver_ApplyCurrentNote(Buzzer buzzer)
{
    BuzzerContext *pContext = &buzzerContexts[buzzer];
    const BuzzerNote *pNote = &pContext->pNotes[pContext->currentNoteIndex];

    if (0u == pNote->frequencyHz)
    {
        // Пауза: CCR = 0, ARR не трогаем, callback НЕ вызываем
        BuzzersDriver_SetCcr(buzzer, BUZZER_CCR_SILENCE);

        return;
    }

    // Нота: устанавливаем ARR/CCR
    BuzzersDriver_ApplyFrequency(buzzer, pNote->frequencyHz);

    // Callback — ПОСЛЕ установки ARR/CCR
    if (pContext->pNoteStartCallback != NULL)
    {
        pContext->pNoteStartCallback(pContext->currentNoteIndex, pNote->frequencyHz);
    }
}

//! \brief Инициализация модуля BuzzersDriver
void BuzzersDriver_Init(void)
{
    static uint8_t initialized = 0u;

    if (initialized != 0u)
    {
        return;
    }

    for (uint8_t index = 0u; index < BUZZERS_QUANTITY; index++)
    {
        buzzerContexts[index].sleeping = 0u;
        buzzerContexts[index].playing = 0u;
        buzzerContexts[index].melodyActive = 0u;
        buzzerContexts[index].volumePercent = BUZZER_DEFAULT_VOLUME;
        buzzerContexts[index].pNotes = NULL;
        buzzerContexts[index].notesCount = 0u;
        buzzerContexts[index].currentNoteIndex = 0u;
        buzzerContexts[index].remainingMs = 0u;
        buzzerContexts[index].pMelodyCompleteCallback = NULL;
        buzzerContexts[index].pNoteStartCallback = NULL;
    }

    __HAL_TIM_SET_PRESCALER(&TIM_BUZZERS_HTIM, BUZZER_TIMER_PRESCALER);
    __HAL_TIM_SET_AUTORELOAD(&TIM_BUZZERS_HTIM, 0xFFFFu);
    __HAL_TIM_SET_COUNTER(&TIM_BUZZERS_HTIM, 0u);

    BuzzersDriver_SetCcr(BUZZER_LARGE, BUZZER_CCR_SILENCE);
    BuzzersDriver_SetCcr(BUZZER_SMALL, BUZZER_CCR_SILENCE);

    BuzzersDriver_StartChannel(BUZZER_LARGE);
    BuzzersDriver_StartChannel(BUZZER_SMALL);

    __HAL_TIM_MOE_ENABLE(&TIM_BUZZERS_HTIM);

    initialized = 1u;
}

//! \brief Обработчик тика, вызывать в прерывании таймера с периодом 100 мкс
void BuzzersDriver_ProcessTick(void)
{
    static uint32_t tickCounter = 0u;

    tickCounter++;

    if (tickCounter < BUZZER_TICK_PERIOD_TICKS)
    {
        return;
    }

    tickCounter = 0u;

    for (uint8_t index = 0u; index < BUZZERS_QUANTITY; index++)
    {
        BuzzerContext *pContext = &buzzerContexts[index];

        if (0u == pContext->playing)
        {
            continue;
        }

        if (0u == pContext->melodyActive)
        {
            // Одиночный тон с длительностью
            if (0u == pContext->remainingMs)
            {
                continue;
            }

            pContext->remainingMs--;

            if (0u == pContext->remainingMs)
            {
                BuzzersDriver_Stop((Buzzer)index);
            }

            continue;
        }

        // Мелодия: отсчёт длительности текущей ноты
        if (pContext->remainingMs > 0u)
        {
            pContext->remainingMs--;
        }

        if (pContext->remainingMs > 0u)
        {
            continue;
        }

        // Переход к следующей ноте
        pContext->currentNoteIndex++;

        if (pContext->currentNoteIndex >= pContext->notesCount)
        {
            // Мелодия завершена
            void (*pCompleteCallback)(void) = pContext->pMelodyCompleteCallback;

            BuzzersDriver_ResetMelody((Buzzer)index);
            pContext->playing = 0u;

            BuzzersDriver_SetCcr((Buzzer)index, BUZZER_CCR_SILENCE);

            if (pCompleteCallback != NULL)
            {
                pCompleteCallback();
            }

            continue;
        }

        pContext->remainingMs = pContext->pNotes[pContext->currentNoteIndex].durationMs;

        // Применяем ноту и уведомляем callback (для нот, не для пауз)
        BuzzersDriver_ApplyCurrentNote((Buzzer)index);
    }
}

//! \brief Запуск тона на одном зуммере
void BuzzersDriver_PlayTone(Buzzer buzzer, uint16_t frequencyHz, uint16_t durationMs)
{
    if (0u == BuzzersDriver_IsValidBuzzer(buzzer))
    {
        return;
    }

    if (0u != buzzerContexts[buzzer].sleeping)
    {
        return;
    }

    if (0u == frequencyHz)
    {
        BuzzersDriver_Stop(buzzer);

        return;
    }

    if ((frequencyHz < BUZZER_MIN_FREQUENCY_HZ) ||
        (frequencyHz > BUZZER_MAX_FREQUENCY_HZ))
    {
        return;
    }

    BuzzersDriver_ResetMelody(buzzer);

    buzzerContexts[buzzer].playing = 1u;
    buzzerContexts[buzzer].remainingMs = durationMs;

    BuzzersDriver_ApplyFrequency(buzzer, frequencyHz);
}

//! \brief Запуск бесконечного тона (до остановки)
void BuzzersDriver_PlayToneContinuous(Buzzer buzzer, uint16_t frequencyHz)
{
    BuzzersDriver_PlayTone(buzzer, frequencyHz, 0u);
}

//! \brief Остановка тона на одном зуммере
void BuzzersDriver_Stop(Buzzer buzzer)
{
    if (0u == BuzzersDriver_IsValidBuzzer(buzzer))
    {
        return;
    }

    BuzzersDriver_ResetMelody(buzzer);

    buzzerContexts[buzzer].playing = 0u;
    buzzerContexts[buzzer].remainingMs = 0u;

    BuzzersDriver_SetCcr(buzzer, BUZZER_CCR_SILENCE);
}

//! \brief Остановка обоих зуммеров
void BuzzersDriver_StopAll(void)
{
    BuzzersDriver_Stop(BUZZER_LARGE);
    BuzzersDriver_Stop(BUZZER_SMALL);
}

//! \brief Проверка, играет ли зуммер
uint8_t BuzzersDriver_IsPlaying(Buzzer buzzer)
{
    if (0u == BuzzersDriver_IsValidBuzzer(buzzer))
    {
        return 0u;
    }

    return buzzerContexts[buzzer].playing;
}

//! \brief Проверка, занят ли хотя бы один зуммер
uint8_t BuzzersDriver_IsAnyPlaying(void)
{
    for (uint8_t index = 0u; index < BUZZERS_QUANTITY; index++)
    {
        if (0u != buzzerContexts[index].playing)
        {
            return 1u;
        }
    }

    return 0u;
}

//! \brief Запуск мелодии (неблокирующе)
void BuzzersDriver_PlayMelody(Buzzer buzzer, const BuzzerNote *pNotes, uint16_t notesCount)
{
    if (0u == BuzzersDriver_IsValidBuzzer(buzzer))
    {
        return;
    }

    if (0u != buzzerContexts[buzzer].sleeping)
    {
        return;
    }

    if ((pNotes == NULL) || (notesCount == 0u))
    {
        return;
    }

    BuzzersDriver_ResetMelody(buzzer);

    buzzerContexts[buzzer].pNotes = pNotes;
    buzzerContexts[buzzer].notesCount = notesCount;
    buzzerContexts[buzzer].currentNoteIndex = 0u;
    buzzerContexts[buzzer].remainingMs = pNotes[0].durationMs;
    buzzerContexts[buzzer].melodyActive = 1u;
    buzzerContexts[buzzer].playing = 1u;

    // Первая нота: применяем немедленно (ARR/CCR + callback для нот)
    BuzzersDriver_ApplyCurrentNote(buzzer);
}

//! \brief Установка callback по завершении мелодии
void BuzzersDriver_SetMelodyCompleteCallback(Buzzer buzzer, void (*pCallback)(void))
{
    if (0u == BuzzersDriver_IsValidBuzzer(buzzer))
    {
        return;
    }

    buzzerContexts[buzzer].pMelodyCompleteCallback = pCallback;
}

//! \brief Установка callback по началу ноты
void BuzzersDriver_SetNoteStartCallback(Buzzer buzzer, void (*pCallback)(uint16_t noteIndex, uint16_t frequencyHz))
{
    if (0u == BuzzersDriver_IsValidBuzzer(buzzer))
    {
        return;
    }

    buzzerContexts[buzzer].pNoteStartCallback = pCallback;
}

//! \brief Получить индекс текущей ноты мелодии
uint16_t BuzzersDriver_GetCurrentNoteIndex(Buzzer buzzer)
{
    if (0u == BuzzersDriver_IsValidBuzzer(buzzer))
    {
        return BUZZER_NOTE_INDEX_NONE;
    }

    const BuzzerContext *pContext = &buzzerContexts[buzzer];

    if (0u == pContext->melodyActive)
    {
        return BUZZER_NOTE_INDEX_NONE;
    }

    if (pContext->pNotes == NULL)
    {
        return BUZZER_NOTE_INDEX_NONE;
    }

    if (pContext->currentNoteIndex >= pContext->notesCount)
    {
        return BUZZER_NOTE_INDEX_NONE;
    }

    return pContext->currentNoteIndex;
}

//! \brief Получить частоту текущей ноты мелодии
uint16_t BuzzersDriver_GetCurrentNoteFrequency(Buzzer buzzer)
{
    if (0u == BuzzersDriver_IsValidBuzzer(buzzer))
    {
        return 0u;
    }

    const BuzzerContext *pContext = &buzzerContexts[buzzer];

    if (0u == pContext->melodyActive)
    {
        return 0u;
    }

    if (pContext->pNotes == NULL)
    {
        return 0u;
    }

    if (pContext->currentNoteIndex >= pContext->notesCount)
    {
        return 0u;
    }

    return pContext->pNotes[pContext->currentNoteIndex].frequencyHz;
}

//! \brief Получение частоты ноты по идентификатору
uint16_t BuzzersDriver_GetNoteFrequency(BuzzerNoteId noteId)
{
    if (noteId >= BUZZER_NOTES_COUNT)
    {
        return 0u;
    }

    return buzzerNoteFrequencies[noteId];
}

//! \brief Установка громкости
void BuzzersDriver_SetVolume(Buzzer buzzer, uint8_t percent)
{
    if (0u == BuzzersDriver_IsValidBuzzer(buzzer))
    {
        return;
    }

    // Спящий зуммер игнорирует все команды, кроме Wake
    if (0u != buzzerContexts[buzzer].sleeping)
    {
        return;
    }

    if (percent > BUZZER_MAX_VOLUME_PERCENT)
    {
        percent = BUZZER_MAX_VOLUME_PERCENT;
    }

    buzzerContexts[buzzer].volumePercent = percent;

    if (0u == buzzerContexts[buzzer].playing)
    {
        return;
    }

    if (0u != BuzzersDriver_IsCurrentNoteRest(buzzer))
    {
        return;
    }

    uint16_t arr = (uint16_t)__HAL_TIM_GET_AUTORELOAD(&TIM_BUZZERS_HTIM);
    uint16_t ccr = BuzzersDriver_CalculateCcr(arr, percent);

    BuzzersDriver_SetCcr(buzzer, ccr);
}

//! \brief Чтение громкости
uint8_t BuzzersDriver_GetVolume(Buzzer buzzer)
{
    if (0u == BuzzersDriver_IsValidBuzzer(buzzer))
    {
        return 0u;
    }

    return buzzerContexts[buzzer].volumePercent;
}

//! \brief Перевод зуммера в режим пониженного потребления
void BuzzersDriver_Sleep(Buzzer buzzer)
{
    if (0u == BuzzersDriver_IsValidBuzzer(buzzer))
    {
        return;
    }

    BuzzersDriver_Stop(buzzer);

    buzzerContexts[buzzer].sleeping = 1u;
}

//! \brief Восстановление зуммера после режима пониженного потребления
void BuzzersDriver_Wake(Buzzer buzzer)
{
    if (0u == BuzzersDriver_IsValidBuzzer(buzzer))
    {
        return;
    }

    buzzerContexts[buzzer].sleeping = 0u;
}

//! \brief Перевод обоих зуммеров в сон
void BuzzersDriver_SleepAll(void)
{
    BuzzersDriver_Sleep(BUZZER_LARGE);
    BuzzersDriver_Sleep(BUZZER_SMALL);
}

//! \brief Спецзвук «клик»
void BuzzersDriver_Click(Buzzer buzzer)
{
    BuzzersDriver_PlayTone(buzzer, 2000u, 10u);
}

//! \brief Спецзвук «ошибка»
void BuzzersDriver_Error(Buzzer buzzer)
{
    BuzzersDriver_PlayTone(buzzer, 200u, 300u);
}

//! \brief Спецзвук «успех» (восходящая мелодия C5–E5–G5)
void BuzzersDriver_Success(Buzzer buzzer)
{
    static const BuzzerNote successMelody[] =
    {
        { 523u, 80u },   //!< C5
        { 659u, 80u },   //!< E5
        { 784u, 80u }    //!< G5
    };

    BuzzersDriver_PlayMelody(buzzer, successMelody, (uint16_t)(sizeof(successMelody) / sizeof(successMelody[0])));
}
