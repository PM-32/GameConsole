#ifndef INC_SYNC_PERIPHERY_H_
#define INC_SYNC_PERIPHERY_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>

#include "BuzzersDriver.h"
#include "LedsDriver.h"

//! Период обновления модуля, мс
#define SYNC_PERIPHERY_UPDATE_PERIOD_MS     20u

//! Максимальное количество одновременных вспышек
#define SYNC_PERIPHERY_FLASH_COUNT_MAX      LEDS_QUANTITY

//! Длительность вспышки «клик», мс
#define SYNC_PERIPHERY_CLICK_FLASH_MS       200u

//! Длительность вспышки «попадание», мс
#define SYNC_PERIPHERY_HIT_FLASH_MS         200u

//! Длительность вспышки «промах», мс
#define SYNC_PERIPHERY_MISS_FLASH_MS        300u

//! Длительность вспышки «ошибка», мс
#define SYNC_PERIPHERY_ERROR_FLASH_MS       300u

//! Длительность эффекта «успех», мс
#define SYNC_PERIPHERY_SUCCESS_FLASH_MS     3000u

//! Красная компонента вспышки «клик»
#define SYNC_PERIPHERY_CLICK_RED            255u

//! Зелёная компонента вспышки «клик»
#define SYNC_PERIPHERY_CLICK_GREEN          255u

//! Синяя компонента вспышки «клик»
#define SYNC_PERIPHERY_CLICK_BLUE           255u

//! Красная компонента вспышки «попадание»
#define SYNC_PERIPHERY_HIT_RED              0u

//! Зелёная компонента вспышки «попадание»
#define SYNC_PERIPHERY_HIT_GREEN            255u

//! Синяя компонента вспышки «попадание»
#define SYNC_PERIPHERY_HIT_BLUE             0u

//! Красная компонента вспышки «промах»
#define SYNC_PERIPHERY_MISS_RED             255u

//! Зелёная компонента вспышки «промах»
#define SYNC_PERIPHERY_MISS_GREEN           0u

//! Синяя компонента вспышки «промах»
#define SYNC_PERIPHERY_MISS_BLUE            0u

//! Красная компонента вспышки «ошибка»
#define SYNC_PERIPHERY_ERROR_RED            255u

//! Зелёная компонента вспышки «ошибка»
#define SYNC_PERIPHERY_ERROR_GREEN          0u

//! Синяя компонента вспышки «ошибка»
#define SYNC_PERIPHERY_ERROR_BLUE           0u

//! Допуск поиска ближайшей частоты, Гц
#define SYNC_PERIPHERY_FREQ_TOLERANCE_HZ    10u

//! Громкость акцента «попадание», %
#define SYNC_PERIPHERY_HIT_VOLUME_PERCENT   10u

//! Длительность акцента «попадание», мс
#define SYNC_PERIPHERY_HIT_DURATION_MS      20u

//! Частота акцента «попадание», Гц
#define SYNC_PERIPHERY_HIT_FREQUENCY_HZ     2000u

//! Громкость клика, %
//! \note SyncPeriphery_Click устанавливает громкость и НЕ восстанавливает.
//!       В тестовом режиме это сделано для слышимости клика.
//!       При интеграции в игры — либо убрать SetVolume из Click,
//!       либо явно выставлять громкость в игре перед PlayNote/Hit.
#define SYNC_PERIPHERY_CLICK_VOLUME_PERCENT 30u

//! Длительность клика, мс
#define SYNC_PERIPHERY_CLICK_DURATION_MS    30u

//! Частота клика, Гц
#define SYNC_PERIPHERY_CLICK_FREQUENCY_HZ   2000u

//! \brief Состояние одной вспышки
typedef struct
{
    uint8_t  active;        //!< Флаг активности вспышки
    uint8_t  red;           //!< Красная компонента
    uint8_t  green;         //!< Зелёная компонента
    uint8_t  blue;          //!< Синяя компонента
    uint16_t remainingMs;   //!< Остаток времени вспышки, мс
    uint16_t totalMs;       //!< Полная длительность вспышки, мс
} SyncPeripheryFlash;

//! \brief Инициализация модуля SyncPeriphery
//! \note Идемпотентна. После инициализации нет активных вспышек.
//! \note Перед вызовом SyncPeriphery_Init() должны быть вызваны:
//!       - LedsDriver_Init()
//!       - LedsEffects_Init()
//!       - BuzzersDriver_Init()
void SyncPeriphery_Init(void);

//! \brief Обновление состояния вспышек
//! \note Неблокирующая. Декрементирует remainingMs, пересчитывает яркость,
//!       вызывает LedsDriver_SetColor() для активных вспышек.
//!       НЕ вызывает LedsDriver_Show().
//! \note Порядок вызова в задаче-владельце LED:
//!       LedsEffects_Update() → SyncPeriphery_Update() → LedsDriver_Show().
void SyncPeriphery_Update(void);

//! \brief Отклик «клик» (кнопка нажата)
//! \note Звук: BuzzersDriver_PlayTone(BUZZER_SMALL, 2000 Гц, 30 мс).
//!       Устанавливает громкость маленького зуммера в SYNC_PERIPHERY_CLICK_VOLUME_PERCENT.
//!       Свет: вспышка всех LED белым (200 мс).
void SyncPeriphery_Click(void);

//! \brief Отклик «нота»
//! \param[in] buzzer Идентификатор зуммера
//! \param[in] frequencyHz Частота ноты, Гц (0 = пауза, свет не запускается)
//! \param[in] durationMs Длительность ноты, мс
//! \note Звук: BuzzersDriver_PlayTone(buzzer, frequencyHz, durationMs).
//!       Свет: вспышка всех LED цветом ноты на durationMs.
void SyncPeriphery_PlayNote(Buzzer buzzer, uint16_t frequencyHz, uint16_t durationMs);

//! \brief Остановка ноты
//! \param[in] buzzer Идентификатор зуммера
//! \note Останавливает только звук. Вспышка LED продолжает затухать
//!       естественным образом через SyncPeriphery_Update().
void SyncPeriphery_StopNote(Buzzer buzzer);

//! \brief Отклик «успех»
//! \note Звук: BuzzersDriver_Success(BUZZER_SMALL).
//!       Свет: зелёная волна через LedsEffects (PROGRESS, приоритет NORMAL).
//! \note Для работы эффекта задача-владелец LED обязана вызывать
//!       LedsEffects_Update() каждый тик.
void SyncPeriphery_Success(void);

//! \brief Отклик «ошибка»
//! \note Звук: BuzzersDriver_Error(BUZZER_SMALL) (200 Гц, 300 мс).
//!       Свет: красная вспышка всех LED (300 мс).
void SyncPeriphery_Error(void);

//! \brief Отклик «попадание»
//! \note Звук: акцент маленького зуммера (громкость 10%, 2000 Гц, 20 мс).
//!       Свет: зелёная вспышка всех LED (200 мс).
//! \note SyncPeriphery_Hit() устанавливает громкость маленького зуммера
//!       в SYNC_PERIPHERY_HIT_VOLUME_PERCENT и НЕ восстанавливает её.
//!       Приложение само управляет громкостью зуммеров.
void SyncPeriphery_Hit(void);

//! \brief Отклик «промах»
//! \note Звук: тишина.
//!       Свет: красная вспышка всех LED (300 мс).
void SyncPeriphery_Miss(void);

//! \brief Преобразование частоты в цвет
//! \param[in] frequencyHz Частота, Гц
//! \param[out] pRed Красная компонента
//! \param[out] pGreen Зелёная компонента
//! \param[out] pBlue Синяя компонента
//! \note Фиксированный маппинг C4–B5 без интерполяции.
//!       Поиск ближайшей частоты с допуском ±10 Гц.
//!       Частоты вне диапазона — белый (255, 255, 255).
void SyncPeriphery_FrequencyToColor(uint16_t frequencyHz,
                                    uint8_t *pRed, uint8_t *pGreen, uint8_t *pBlue);

//! \brief Запуск вспышки на одном LED
//! \param[in] ledIndex Индекс светодиода
//! \param[in] red Красная компонента
//! \param[in] green Зелёная компонента
//! \param[in] blue Синяя компонента
//! \param[in] durationMs Длительность вспышки, мс (0 = игнорируется)
//! \note Один активный отклик на LED — последний побеждает (замена).
void SyncPeriphery_StartFlash(uint8_t ledIndex,
                              uint8_t red, uint8_t green, uint8_t blue,
                              uint16_t durationMs);

//! \brief Остановка всех вспышек
//! \note Гасит все LED, на которых были активные вспышки
//!       (устанавливает цвет 0,0,0 через LedsDriver_SetColor).
//!       Для применения к физической ленте вызывающий должен
//!       вызвать LedsDriver_Show().
void SyncPeriphery_StopAllFlashes(void);

//! \brief Проверка наличия активных вспышек
//! \return 1, если есть хотя бы одна активная вспышка, иначе 0
uint8_t SyncPeriphery_HasActiveFlashes(void);

#ifdef __cplusplus
}
#endif

#endif // INC_SYNC_PERIPHERY_H_
