#include "FreeRTOS.h"
#include "task.h"

#include "BuzzersControl.h"
#include "BuzzersDriver.h"

//! Размер стека задачи тестирования зуммеров, слов
#define BUZZERS_CONTROL_TASK_STACK_SIZE     (configMINIMAL_STACK_SIZE)

//! Приоритет задачи тестирования зуммеров
#define BUZZERS_CONTROL_TASK_PRIORITY       (tskIDLE_PRIORITY + 1u)

//! Пауза между воспроизведениями, мс
#define BUZZERS_CONTROL_PAUSE_MS            5000u

//! Громкость большого зуммера при совместном воспроизведении, %
#define BUZZERS_CONTROL_LARGE_VOLUME_UNISON 2u

//! Громкость маленького зуммера при совместном воспроизведении, %
#define BUZZERS_CONTROL_SMALL_VOLUME_UNISON 4u

//! Длительность мелодии, мс.
//! Ноты 1–15: 200 мс × 15 = 3000 мс.
//! Нота 16: 400 мс.
//! Ноты 17–19: 200 мс × 3 = 600 мс.
//! Нота 20: 400 мс.
//! Итого: 3000 + 400 + 600 + 400 = 4400 мс.
#define BUZZERS_CONTROL_MELODY_DURATION_MS  4400u

//! Мелодия, исполняемая большим зуммером (20 нот).
//! Мелодия на основе «Оды к радости» (Бетховен).
static const BuzzerNote buzzersControlFullMelody[] =
{
    { 523u, 200u },  //!< 1:  C5
    { 523u, 200u },  //!< 2:  C5
    { 587u, 200u },  //!< 3:  D5
    { 659u, 200u },  //!< 4:  E5
    { 659u, 200u },  //!< 5:  E5
    { 587u, 200u },  //!< 6:  D5
    { 523u, 200u },  //!< 7:  C5
    { 494u, 200u },  //!< 8:  B4
    { 440u, 200u },  //!< 9:  A4
    { 440u, 200u },  //!< 10: A4
    { 494u, 200u },  //!< 11: B4
    { 523u, 200u },  //!< 12: C5
    { 523u, 200u },  //!< 13: C5
    { 494u, 200u },  //!< 14: B4
    { 494u, 200u },  //!< 15: B4
    { 0u,   400u },  //!< 16: пауза
    { 440u, 200u },  //!< 17: A4
    { 440u, 200u },  //!< 18: A4
    { 494u, 200u },  //!< 19: B4
    { 523u, 400u }   //!< 20: C5 (длинная)
};

//! Количество нот в мелодии большого зуммера
#define BUZZERS_CONTROL_FULL_MELODY_NOTES_COUNT \
    ((uint16_t)(sizeof(buzzersControlFullMelody) / sizeof(buzzersControlFullMelody[0])))

//! Мелодия для маленького зуммера при совместном воспроизведении.
//! Маленький звучит в унисон с большим на нотах 5–8 и 13–16,
//! на остальных — пауза. Ритм совпадает с полной мелодией
//! позиция-в-позицию, поэтому ProcessTick двигает обе мелодии
//! синхронно. ARR общий, но у пауз ARR не меняется — конфликта нет.
static const BuzzerNote buzzersControlSmallMelody[] =
{
    { 0u,   200u },  //!< 1:  пауза (звучит только большой: C5)
    { 0u,   200u },  //!< 2:  пауза (звучит только большой: C5)
    { 0u,   200u },  //!< 3:  пауза (звучит только большой: D5)
    { 0u,   200u },  //!< 4:  пауза (звучит только большой: E5)
    { 659u, 200u },  //!< 5:  E5
    { 587u, 200u },  //!< 6:  D5
    { 523u, 200u },  //!< 7:  C5
    { 494u, 200u },  //!< 8:  B4
    { 0u,   200u },  //!< 9:  пауза (звучит только большой: A4)
    { 0u,   200u },  //!< 10: пауза (звучит только большой: A4)
    { 0u,   200u },  //!< 11: пауза (звучит только большой: B4)
    { 0u,   200u },  //!< 12: пауза (звучит только большой: C5)
    { 523u, 200u },  //!< 13: C5
    { 494u, 200u },  //!< 14: B4
    { 494u, 200u },  //!< 15: B4
    { 0u,   400u },  //!< 16: пауза (у обоих пауза)
    { 0u,   200u },  //!< 17: пауза (звучит только большой: A4)
    { 0u,   200u },  //!< 18: пауза (звучит только большой: A4)
    { 0u,   200u },  //!< 19: пауза (звучит только большой: B4)
    { 0u,   400u }   //!< 20: пауза (звучит только большой: C5 длинная)
};

//! Количество нот в мелодии маленького зуммера
#define BUZZERS_CONTROL_SMALL_MELODY_NOTES_COUNT \
    ((uint16_t)(sizeof(buzzersControlSmallMelody) / sizeof(buzzersControlSmallMelody[0])))

//! \brief Задача тестирования зуммеров
//! \details Бесконечный цикл:
//!          1) мелодия на большом зуммере,
//!          2) та же мелодия на маленьком зуммере,
//!          3) большой играет полную мелодию, маленький подыгрывает
//!             в унисон на отдельных нотах (громкость маленького выше).
//!          Между воспроизведениями — пауза BUZZERS_CONTROL_PAUSE_MS.
//!          Задача не использует callback мелодии — ожидание построено
//!          на vTaskDelay с фиксированной длительностью мелодии.
//! \param[in] pvParameters Параметры задачи (не используются)
static void BuzzersControl_Task(void *pvParameters)
{
    (void)pvParameters;

    for (;;)
    {
        // Мелодия на большом зуммере (громкость по умолчанию)
        BuzzersDriver_SetVolume(BUZZER_LARGE, BUZZER_DEFAULT_VOLUME);
        BuzzersDriver_SetVolume(BUZZER_SMALL, BUZZER_DEFAULT_VOLUME);

        BuzzersDriver_PlayMelody(BUZZER_LARGE, buzzersControlFullMelody,
                                 BUZZERS_CONTROL_FULL_MELODY_NOTES_COUNT);
        vTaskDelay(pdMS_TO_TICKS(BUZZERS_CONTROL_MELODY_DURATION_MS));
        vTaskDelay(pdMS_TO_TICKS(BUZZERS_CONTROL_PAUSE_MS));

        // Та же мелодия на маленьком зуммере (громкость по умолчанию)
        BuzzersDriver_SetVolume(BUZZER_LARGE, BUZZER_DEFAULT_VOLUME);
        BuzzersDriver_SetVolume(BUZZER_SMALL, BUZZER_DEFAULT_VOLUME);

        BuzzersDriver_PlayMelody(BUZZER_SMALL, buzzersControlFullMelody,
                                 BUZZERS_CONTROL_FULL_MELODY_NOTES_COUNT);
        vTaskDelay(pdMS_TO_TICKS(BUZZERS_CONTROL_MELODY_DURATION_MS));
        vTaskDelay(pdMS_TO_TICKS(BUZZERS_CONTROL_PAUSE_MS));

        // Совместное воспроизведение: большой играет полную мелодию,
        // маленький подыгрывает в унисон на отдельных нотах.
        // Громкость маленького выше.
        BuzzersDriver_SetVolume(BUZZER_LARGE, BUZZERS_CONTROL_LARGE_VOLUME_UNISON);
        BuzzersDriver_SetVolume(BUZZER_SMALL, BUZZERS_CONTROL_SMALL_VOLUME_UNISON);

        BuzzersDriver_PlayMelody(BUZZER_LARGE, buzzersControlFullMelody,
                                 BUZZERS_CONTROL_FULL_MELODY_NOTES_COUNT);
        BuzzersDriver_PlayMelody(BUZZER_SMALL, buzzersControlSmallMelody,
                                 BUZZERS_CONTROL_SMALL_MELODY_NOTES_COUNT);

        vTaskDelay(pdMS_TO_TICKS(BUZZERS_CONTROL_MELODY_DURATION_MS));
        vTaskDelay(pdMS_TO_TICKS(BUZZERS_CONTROL_PAUSE_MS));
    }
}

//! \brief Создание задачи тестирования зуммеров
//! \return Результат (статус) создания задачи
BaseType_t BuzzersControl_CreateTask(void)
{
    return xTaskCreate(BuzzersControl_Task, "Buzzers",
                       BUZZERS_CONTROL_TASK_STACK_SIZE, NULL,
                       BUZZERS_CONTROL_TASK_PRIORITY,
                       NULL);
}
