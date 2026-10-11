#ifndef INC_LEDS_EFFECTS_H_
#define INC_LEDS_EFFECTS_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>

#include "LedsDriver.h"

#define LEDS_EFFECTS_UPDATE_PERIOD_MS               20u     //!< Период обновления эффектов, мс
#define LEDS_EFFECTS_MAX_DURATION_MS                10000u  //!< Максимальная длительность эффекта, мс
#define LEDS_EFFECTS_STACK_SIZE                     3u      //!< Размер стека вытесненных эффектов

#define LEDS_EFFECTS_DEFAULT_RAINBOW_PERIOD_MS      3000u   //!< Период радуги по умолчанию, мс
#define LEDS_EFFECTS_DEFAULT_BLINK_PERIOD_MS        500u    //!< Период мигания по умолчанию, мс
#define LEDS_EFFECTS_DEFAULT_PULSE_PERIOD_MS        1000u   //!< Период пульсации по умолчанию, мс
#define LEDS_EFFECTS_DEFAULT_KNIGHT_RIDER_PERIOD_MS 1000u   //!< Период бегущего огонька по умолчанию, мс
#define LEDS_EFFECTS_DEFAULT_COLOR_CYCLE_PERIOD_MS  3000u   //!< Период смены цвета по умолчанию, мс
#define LEDS_EFFECTS_DEFAULT_CHASE_PERIOD_MS        1500u   //!< Период погони по умолчанию, мс
#define LEDS_EFFECTS_DEFAULT_SPARKLE_PERIOD_MS      2000u   //!< Период мерцания по умолчанию, мс
#define LEDS_EFFECTS_DEFAULT_THEATER_CHASE_PERIOD_MS 1000u  //!< Период театрального пробега по умолчанию, мс

//! \brief Тип светового эффекта
typedef enum
{
    LEDS_EFFECTS_NONE = 0,          //!< Нет эффекта
    LEDS_EFFECTS_FADE_IN,           //!< Плавное появление
    LEDS_EFFECTS_FADE_OUT,          //!< Плавное исчезновение
    LEDS_EFFECTS_BLINK,             //!< Мигание
    LEDS_EFFECTS_PULSE,             //!< Дыхание
    LEDS_EFFECTS_RAINBOW,           //!< Радуга по ленте (colorMode игнорируется)
    LEDS_EFFECTS_PROGRESS,          //!< Линейное заполнение
    LEDS_EFFECTS_KNIGHT_RIDER,      //!< Бегущий огонёк
    LEDS_EFFECTS_COLOR_CYCLE,       //!< Смена цвета всей ленты (colorMode игнорируется)
    LEDS_EFFECTS_CHASE,             //!< Точка с хвостом
    LEDS_EFFECTS_SPARKLE,           //!< Случайные вспышки
    LEDS_EFFECTS_THEATER_CHASE      //!< Театральный пробег
} LedsEffectsType;

//! \brief Приоритет светового эффекта
typedef enum
{
    LEDS_EFFECTS_PRIORITY_LOW = 0,  //!< Фон (ambient, меню)
    LEDS_EFFECTS_PRIORITY_NORMAL,   //!< Обычные (переходы, пауза)
    LEDS_EFFECTS_PRIORITY_HIGH      //!< Уведомления (прерывают любой)
} LedsEffectsPriority;

//! \brief Режим выбора цвета
//! \note Для LEDS_EFFECTS_RAINBOW и LEDS_EFFECTS_COLOR_CYCLE
//!       (типов эффектов) поле colorMode игнорируется — эти эффекты
//!       сами управляют цветом.
//! \note Имена в этом перечислении отличаются от имён типов эффектов
//!       префиксом COLOR_MODE_ для избежания конфликта идентификаторов
//!       в общем пространстве имён C.
typedef enum
{
    LEDS_EFFECTS_COLOR_SOLID = 0,       //!< Один цвет (red1, green1, blue1)
    LEDS_EFFECTS_COLOR_RAINBOW,         //!< Статичная радуга по ленте
    LEDS_EFFECTS_COLOR_GRADIENT,        //!< Градиент между двумя цветами по ленте (статичный)
    LEDS_EFFECTS_COLOR_MODE_CYCLE,      //!< Циклическая смена цвета всей ленты (между red1 и red2)
    LEDS_EFFECTS_COLOR_RANDOM           //!< Случайные цвета на LED (из предгенерированной таблицы)
} LedsEffectsColorMode;

//! \brief Параметры светового эффекта
typedef struct
{
    LedsEffectsType      type;        //!< Тип эффекта
    LedsEffectsPriority  priority;    //!< Приоритет
    LedsEffectsColorMode colorMode;   //!< Режим цвета

    // Основной цвет (для SOLID, GRADIENT, CYCLE, RANDOM)
    uint8_t              red1;        //!< Красная компонента основного цвета
    uint8_t              green1;      //!< Зелёная компонента основного цвета
    uint8_t              blue1;       //!< Синяя компонента основного цвета

    // Второй цвет (для GRADIENT и CYCLE; игнорируется в других режимах)
    uint8_t              red2;        //!< Красная компонента второго цвета
    uint8_t              green2;      //!< Зелёная компонента второго цвета
    uint8_t              blue2;       //!< Синяя компонента второго цвета

    uint16_t             durationMs;  //!< Длительность (0 = бесконечно)
    uint16_t             periodMs;    //!< Период (0 = по умолчанию)
    uint8_t              count;       //!< Количество повторений (0 = бесконечно)
} LedsEffectsParams;

//! \brief Инициализация модуля световых эффектов
//! \note Идемпотентна. После инициализации нет активного эффекта.
//!       Генерирует таблицу случайных цветов для colorMode = RANDOM.
void LedsEffects_Init(void);

//! \brief Запуск светового эффекта
//! \param[in] pParams - параметры эффекта
//! \note Если приоритет выше текущего — текущий сохраняется в стек.
//!       Если равен — заменяет текущий. Если ниже — игнорируется.
void LedsEffects_Start(const LedsEffectsParams *pParams);

//! \brief Обновление текущего эффекта
//! \note Неблокирующая. Вызывается из задачи-владельца с периодом
//!       LEDS_EFFECTS_UPDATE_PERIOD_MS. Не вызывает LedsDriver_Show().
void LedsEffects_Update(void);

//! \brief Остановка текущего эффекта
//! \note Стек вытесненных эффектов сохраняется.
void LedsEffects_Stop(void);

//! \brief Остановка всех эффектов, включая стек
//! \note Перед вызовом LedsDriver_Sleep() приложение должно
//!       самостоятельно вызвать LedsEffects_StopAll().
void LedsEffects_StopAll(void);

//! \brief Проверка завершения текущего эффекта
//! \return 1 - нет активного эффекта или эффект завершён, 0 - эффект активен
uint8_t LedsEffects_IsFinished(void);

//! \brief Получение приоритета текущего эффекта
//! \return Приоритет текущего эффекта или LEDS_EFFECTS_PRIORITY_LOW, если нет активного
LedsEffectsPriority LedsEffects_GetPriority(void);

//! \brief Получение типа текущего эффекта
//! \return Тип текущего эффекта или LEDS_EFFECTS_NONE, если нет активного
LedsEffectsType LedsEffects_GetType(void);

//! \brief Статичная индикация заряда батареи
//! \param[in] percent - заряд, 0-100 %
//! \note Не является эффектом. Разовая установка цветов через LedsDriver_SetColor.
//!       Не изменяет состояние активного эффекта. Округление количества
//!       горящих LED вверх для ненулевого заряда.
void LedsEffects_ShowBattery(uint8_t percent);

#ifdef __cplusplus
}
#endif

#endif // INC_LEDS_EFFECTS_H_
