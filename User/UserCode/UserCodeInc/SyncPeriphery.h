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
void SyncPeriphery_Init(void);

//! \brief Обновление состояния вспышек
void SyncPeriphery_Update(void);

//! \brief Отклик «клик» (кнопка нажата)
void SyncPeriphery_Click(void);

//! \brief Отклик «нота»
void SyncPeriphery_PlayNote(Buzzer buzzer, uint16_t frequencyHz, uint16_t durationMs);

//! \brief Остановка ноты
void SyncPeriphery_StopNote(Buzzer buzzer);

//! \brief Отклик «успех»
void SyncPeriphery_Success(void);

//! \brief Отклик «ошибка»
void SyncPeriphery_Error(void);

//! \brief Отклик «попадание»
void SyncPeriphery_Hit(void);

//! \brief Отклик «промах»
void SyncPeriphery_Miss(void);

//! \brief Преобразование частоты в цвет
void SyncPeriphery_FrequencyToColor(uint16_t frequencyHz,
                                    uint8_t *pRed, uint8_t *pGreen, uint8_t *pBlue);

//! \brief Запуск вспышки на одном LED
void SyncPeriphery_StartFlash(uint8_t ledIndex,
                              uint8_t red, uint8_t green, uint8_t blue,
                              uint16_t durationMs);

//! \brief Остановка всех вспышек
void SyncPeriphery_StopAllFlashes(void);

//! \brief Проверка наличия активных вспышек
uint8_t SyncPeriphery_HasActiveFlashes(void);

#ifdef __cplusplus
}
#endif

#endif // INC_SYNC_PERIPHERY_H_
