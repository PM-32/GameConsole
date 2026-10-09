#ifndef INC_LEDS_DRIVER_H_
#define INC_LEDS_DRIVER_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>

#define LEDS_QUANTITY                       8u      //!< Количество светодиодов в ленте
#define LEDS_SAFE_BRIGHTNESS_ENABLE         1u      //!< 1 = безопасный режим яркости, 0 = обычный
#define LEDS_SAFE_BRIGHTNESS_MAX_PERCENT    10u     //!< Предел яркости в безопасном режиме, %

//! \brief Состояние одного светодиода
//! \details Порядок полей соответствует порядку передачи WS2812B (GRB).
//!          Изменение порядка полей нарушит формирование DMA-буфера.
typedef struct
{
    uint8_t green;    //!< Зелёная компонента
    uint8_t red;      //!< Красная компонента
    uint8_t blue;     //!< Синяя компонента
} LedState;

//! \brief Инициализация драйвера светодиодов
void LedsDriver_Init(void);

//! \brief Установка цвета одного светодиода
//! \details Изменяет только ledsState. Для передачи на ленту вызвать LedsDriver_Show().
//! \param[in] ledIndex - индекс светодиода
//! \param[in] red - красная компонента
//! \param[in] green - зелёная компонента
//! \param[in] blue - синяя компонента
void LedsDriver_SetColor(uint8_t ledIndex, uint8_t red, uint8_t green, uint8_t blue);

//! \brief Установка цвета всех светодиодов
//! \details Изменяет только ledsState. Для передачи на ленту вызвать LedsDriver_Show().
//! \param[in] red - красная компонента
//! \param[in] green - зелёная компонента
//! \param[in] blue - синяя компонента
void LedsDriver_SetAll(uint8_t red, uint8_t green, uint8_t blue);

//! \brief Гашение всех светодиодов
//! \details Изменяет только ledsState. Для передачи на ленту вызвать LedsDriver_Show().
void LedsDriver_Clear(void);

//! \brief Чтение текущего цвета светодиода
//! \param[in] ledIndex - индекс светодиода
//! \param[out] pRed - красная компонента
//! \param[out] pGreen - зелёная компонента
//! \param[out] pBlue - синяя компонента
void LedsDriver_GetColor(uint8_t ledIndex, uint8_t *pRed, uint8_t *pGreen, uint8_t *pBlue);

//! \brief Получение состояния одного светодиода
//! \param[in] ledIndex - индекс светодиода
//! \param[out] pLedState - состояние светодиода
void LedsDriver_GetLedState(uint8_t ledIndex, LedState *pLedState);

//! \brief Применение буфера к ленте (запуск передачи через DMA)
//! \details Неблокирующая. Если предыдущая передача не завершена — вызов игнорируется.
//!          При ошибке запуска DMA флаг передачи сбрасывается.
void LedsDriver_Show(void);

//! \brief Установка яркости в процентах
//! \param[in] percent - яркость, 0-100 %
void LedsDriver_SetBrightness(uint8_t percent);

//! \brief Чтение текущей яркости
//! \return Яркость, 0-100 %
uint8_t LedsDriver_GetBrightness(void);

//! \brief Перевод ленты в режим пониженного потребления
//! \details Сохраняет текущее ledsState, гасит ленту и применяет состояние
//!          к физической ленте (внутренний вызов LedsDriver_Show()).
void LedsDriver_Sleep(void);

//! \brief Восстановление ленты после режима пониженного потребления
//! \details Восстанавливает ledsState из сохранённого и применяет состояние
//!          к физической ленте (внутренний вызов LedsDriver_Show()).
void LedsDriver_Wake(void);

//! \brief Установка callback по завершении передачи
//! \param[in] pCallback - указатель на callback-функцию
void LedsDriver_SetTransferCompleteCallback(void (*pCallback)(void));

//! \brief Установка «радуги» для ambient-режима
//! \details Изменяет только ledsState. Для передачи на ленту вызвать LedsDriver_Show().
//! \param[in] offset - смещение оттенка
void LedsDriver_SetRainbow(uint8_t offset);

//! \brief Callback завершения передачи DMA (для HAL_TIM_PWM_PulseFinishedCallback)
//! \details Вызывается из SystemCallbacks.c при завершении генерации ШИМ.
void LedsDriver_TransferCompleteCallback(void);

#ifdef __cplusplus
}
#endif

#endif // INC_LEDS_DRIVER_H_
