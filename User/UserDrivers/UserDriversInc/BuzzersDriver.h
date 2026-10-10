#ifndef INC_BUZZERS_DRIVER_H_
#define INC_BUZZERS_DRIVER_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>

//! Количество зуммеров в системе
#define BUZZERS_QUANTITY              2u

//! Громкость по умолчанию, %
#define BUZZER_DEFAULT_VOLUME         5u

//! Минимальная поддерживаемая частота, Гц
#define BUZZER_MIN_FREQUENCY_HZ       100u

//! Максимальная поддерживаемая частота, Гц
#define BUZZER_MAX_FREQUENCY_HZ       5000u

//! Предделитель таймера зуммеров (72 МГц / (71 + 1) = 1 МГц)
#define BUZZER_TIMER_PRESCALER        71u

//! Частота счёта таймера зуммеров после предделителя, Гц
#define BUZZER_TIMER_TICK_HZ          1000000u

//! Значение, возвращаемое GetCurrentNoteIndex, если мелодия не активна
#define BUZZER_NOTE_INDEX_NONE        0xFFFFu

//! \brief Идентификаторы зуммеров
typedef enum
{
    BUZZER_LARGE = 0,    //!< Большой зуммер (PA8, TIM1_CH1)
    BUZZER_SMALL,        //!< Маленький зуммер (PA9, TIM1_CH2)
    BUZZERS_COUNT        //!< Маркер количества
} Buzzer;

//! \brief Описание одной ноты мелодии
typedef struct
{
    uint16_t frequencyHz;   //!< Частота ноты (0 = пауза)
    uint16_t durationMs;    //!< Длительность ноты, мс (0 = нота пропускается)
} BuzzerNote;

//! \brief Идентификаторы нот (две октавы C4–B5 + пауза)
typedef enum
{
    BUZZER_NOTE_C4 = 0,     //!< Нота C4 (до первой октавы)
    BUZZER_NOTE_CS4,        //!< Нота C#4 (до-диез первой октавы)
    BUZZER_NOTE_D4,         //!< Нота D4 (ре первой октавы)
    BUZZER_NOTE_DS4,        //!< Нота D#4 (ре-диез первой октавы)
    BUZZER_NOTE_E4,         //!< Нота E4 (ми первой октавы)
    BUZZER_NOTE_F4,         //!< Нота F4 (фа первой октавы)
    BUZZER_NOTE_FS4,        //!< Нота F#4 (фа-диез первой октавы)
    BUZZER_NOTE_G4,         //!< Нота G4 (соль первой октавы)
    BUZZER_NOTE_GS4,        //!< Нота G#4 (соль-диез первой октавы)
    BUZZER_NOTE_A4,         //!< Нота A4 (ля первой октавы)
    BUZZER_NOTE_AS4,        //!< Нота A#4 (ля-диез первой октавы)
    BUZZER_NOTE_B4,         //!< Нота B4 (си первой октавы)

    BUZZER_NOTE_C5,         //!< Нота C5 (до второй октавы)
    BUZZER_NOTE_CS5,        //!< Нота C#5 (до-диез второй октавы)
    BUZZER_NOTE_D5,         //!< Нота D5 (ре второй октавы)
    BUZZER_NOTE_DS5,        //!< Нота D#5 (ре-диез второй октавы)
    BUZZER_NOTE_E5,         //!< Нота E5 (ми второй октавы)
    BUZZER_NOTE_F5,         //!< Нота F5 (фа второй октавы)
    BUZZER_NOTE_FS5,        //!< Нота F#5 (фа-диез второй октавы)
    BUZZER_NOTE_G5,         //!< Нота G5 (соль второй октавы)
    BUZZER_NOTE_GS5,        //!< Нота G#5 (соль-диез второй октавы)
    BUZZER_NOTE_A5,         //!< Нота A5 (ля второй октавы)
    BUZZER_NOTE_AS5,        //!< Нота A#5 (ля-диез второй октавы)
    BUZZER_NOTE_B5,         //!< Нота B5 (си второй октавы)

    BUZZER_NOTE_REST,       //!< Пауза (тишина)
    BUZZER_NOTES_COUNT      //!< Маркер количества нот
} BuzzerNoteId;

//! \brief Инициализация модуля BuzzersDriver
//! \note  В CubeMX для TIM1 обязательно должно быть выставлено
//!        Auto-Reload Preload = Enable. Иначе смена ARR применится
//!        немедленно, что даст щелчки при смене частоты (N-08).
void BuzzersDriver_Init(void);

//! \brief Обработчик тика, вызывать в прерывании таймера с периодом 100 мкс
//! \details Отсчитывает длительности нот и одиночных тонов, переключает ноты.
void BuzzersDriver_ProcessTick(void);

//! \brief Запуск тона на одном зуммере
//! \param[in] buzzer Идентификатор зуммера
//! \param[in] frequencyHz Частота тона, Гц (0 = стоп)
//! \param[in] durationMs Длительность тона, мс (0 = бесконечно)
//! \warning ARR общий для обоих каналов TIM1. Если второй зуммер
//!          активен, его частота также изменится. Приложение должно
//!          само следить за конфликтами (например, не запускать клик
//!          во время мелодии).
void BuzzersDriver_PlayTone(Buzzer buzzer, uint16_t frequencyHz, uint16_t durationMs);

//! \brief Запуск бесконечного тона (до остановки)
//! \param[in] buzzer Идентификатор зуммера
//! \param[in] frequencyHz Частота тона, Гц
//! \warning ARR общий для обоих каналов TIM1. См. BuzzersDriver_PlayTone.
void BuzzersDriver_PlayToneContinuous(Buzzer buzzer, uint16_t frequencyHz);

//! \brief Остановка тона на одном зуммере
//! \param[in] buzzer Идентификатор зуммера
void BuzzersDriver_Stop(Buzzer buzzer);

//! \brief Остановка обоих зуммеров
void BuzzersDriver_StopAll(void);

//! \brief Проверка, играет ли зуммер
//! \param[in] buzzer Идентификатор зуммера
//! \return 1, если зуммер занят (в том числе паузой в мелодии), иначе 0
uint8_t BuzzersDriver_IsPlaying(Buzzer buzzer);

//! \brief Проверка, занят ли хотя бы один зуммер
//! \return 1, если хотя бы один зуммер занят, иначе 0
uint8_t BuzzersDriver_IsAnyPlaying(void);

//! \brief Запуск мелодии (неблокирующе)
//! \param[in] buzzer Идентификатор зуммера
//! \param[in] pNotes Указатель на массив нот (должен жить всё время воспроизведения)
//! \param[in] notesCount Количество нот
//! \note  Нота с durationMs = 0 будет пропущена (переход к следующей
//!        произойдёт в следующем тике ProcessTick).
//! \warning ARR общий для обоих каналов TIM1. См. BuzzersDriver_PlayTone.
void BuzzersDriver_PlayMelody(Buzzer buzzer, const BuzzerNote *pNotes, uint16_t notesCount);

//! \brief Установка callback по завершении мелодии
//! \param[in] buzzer Идентификатор зуммера
//! \param[in] pCallback Указатель на callback (вызывается из ISR, должен быть коротким)
//! \warning Callback не должен вызывать функции того же зуммера
//!          (Stop, PlayMelody, PlayTone) — это нарушит состояние ProcessTick.
//!          Для остановки — уведомить задачу через vTaskNotifyGiveFromISR
//!          и вызвать Stop из задачи.
void BuzzersDriver_SetMelodyCompleteCallback(Buzzer buzzer, void (*pCallback)(void));

//! \brief Установка callback по началу ноты
//! \details Callback вызывается из ISR (ProcessTick), должен быть коротким.
//!          Для передачи управления задаче использовать vTaskNotifyGiveFromISR.
//!          Callback вызывается ТОЛЬКО для нот (frequencyHz != 0), для пауз — нет.
//!          Для первой ноты мелодии callback вызывается сразу из PlayMelody,
//!          для последующих — из ProcessTick при переходе к следующей ноте.
//!          Callback вызывается ПОСЛЕ установки ARR и CCR для ноты.
//! \param[in] buzzer Идентификатор зуммера
//! \param[in] pCallback Указатель на callback (noteIndex, frequencyHz)
//! \warning Callback не должен вызывать функции того же зуммера
//!          (Stop, PlayMelody, PlayTone) — это нарушит состояние ProcessTick.
//!          Для остановки — уведомить задачу через vTaskNotifyGiveFromISR
//!          и вызвать Stop из задачи.
void BuzzersDriver_SetNoteStartCallback(Buzzer buzzer, void (*pCallback)(uint16_t noteIndex, uint16_t frequencyHz));

//! \brief Получить индекс текущей ноты мелодии
//! \param[in] buzzer Идентификатор зуммера
//! \return Индекс текущей ноты (0-based) или BUZZER_NOTE_INDEX_NONE, если мелодия не активна
uint16_t BuzzersDriver_GetCurrentNoteIndex(Buzzer buzzer);

//! \brief Получить частоту текущей ноты мелодии
//! \param[in] buzzer Идентификатор зуммера
//! \return Частота текущей ноты, Гц (0 для паузы или если мелодия не активна)
uint16_t BuzzersDriver_GetCurrentNoteFrequency(Buzzer buzzer);

//! \brief Получение частоты ноты по идентификатору
//! \param[in] noteId Идентификатор ноты
//! \return Частота ноты, Гц (0 для паузы или при некорректном идентификаторе)
uint16_t BuzzersDriver_GetNoteFrequency(BuzzerNoteId noteId);

//! \brief Установка громкости
//! \param[in] buzzer Идентификатор зуммера
//! \param[in] percent Громкость, 0–100 % (100 = максимум, duty 50 %)
//! \note  Для спящего зуммера вызов игнорируется (см. BuzzersDriver_Sleep).
void BuzzersDriver_SetVolume(Buzzer buzzer, uint8_t percent);

//! \brief Чтение громкости
//! \param[in] buzzer Идентификатор зуммера
//! \return Громкость, 0–100 %
uint8_t BuzzersDriver_GetVolume(Buzzer buzzer);

//! \brief Перевод зуммера в режим пониженного потребления
//! \param[in] buzzer Идентификатор зуммера
void BuzzersDriver_Sleep(Buzzer buzzer);

//! \brief Восстановление зуммера после режима пониженного потребления
//! \param[in] buzzer Идентификатор зуммера
void BuzzersDriver_Wake(Buzzer buzzer);

//! \brief Перевод обоих зуммеров в сон
void BuzzersDriver_SleepAll(void);

//! \brief Спецзвук «клик»
//! \param[in] buzzer Идентификатор зуммера
void BuzzersDriver_Click(Buzzer buzzer);

//! \brief Спецзвук «ошибка»
//! \param[in] buzzer Идентификатор зуммера
void BuzzersDriver_Error(Buzzer buzzer);

//! \brief Спецзвук «успех» (восходящая мелодия C5–E5–G5)
//! \param[in] buzzer Идентификатор зуммера
void BuzzersDriver_Success(Buzzer buzzer);

#ifdef __cplusplus
}
#endif

#endif // INC_BUZZERS_DRIVER_H_
