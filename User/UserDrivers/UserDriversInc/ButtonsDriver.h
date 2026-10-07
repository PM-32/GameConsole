#ifndef INC_BUTTONS_DRIVER_H_
#define INC_BUTTONS_DRIVER_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>

#include "UserTimer.h"

//! Количество кнопок в системе
#define BUTTONS_DRIVER_BUTTON_COUNT          8u

//! Период вызова обработчика в микросекундах
#define BUTTONS_DRIVER_TICK_MICROSECONDS     100u

//! Время антидребезга в тиках (5 мс / 100 мкс)
#define BUTTONS_DRIVER_DEBOUNCE_TICKS        50u

//! Порог длинного нажатия в тиках (1000 мс / 100 мкс)
#define BUTTONS_DRIVER_LONG_PRESS_TICKS      10000u

//! Порог длинного нажатия в миллисекундах
#define BUTTONS_DRIVER_LONG_PRESS_MS         (BUTTONS_DRIVER_LONG_PRESS_TICKS / USER_TIMER_PERIODS_IN_1_MS)

//! Окно ожидания второго нажатия в тиках (200 мс / 100 мкс)
#define BUTTONS_DRIVER_SECOND_CLICK_TICKS    2000u

//! Период автоповтора при удержании в тиках (1000 мс / 100 мкс)
#define BUTTONS_DRIVER_REPEAT_PERIOD_TICKS   10000u

//! \brief Идентификаторы кнопок
typedef enum
{
    BUTTON_1 = 0,       //!< Кнопка 1
    BUTTON_2,           //!< Кнопка 2
    BUTTON_3,           //!< Кнопка 3
    BUTTON_4,           //!< Кнопка 4
    BUTTON_5,           //!< Кнопка 5
    BUTTON_6,           //!< Кнопка 6
    BUTTON_7,           //!< Кнопка 7
    BUTTON_8,           //!< Кнопка 8
    BUTTONS_QUANTITY    //!< Количество кнопок (маркер)
} Buttons;

//! \brief Состояния конечного автомата кнопки
typedef enum
{
    BUTTONS_DRIVER_STATE_RELEASED = 0,      //!< Кнопка отпущена, стабильно
    BUTTONS_DRIVER_STATE_DEBOUNCE_DOWN,     //!< Кандидат на нажатие, идёт антидребезг
    BUTTONS_DRIVER_STATE_PRESSED,           //!< Кнопка нажата, идёт отсчёт удержания
    BUTTONS_DRIVER_STATE_DEBOUNCE_UP,       //!< Кандидат на отпускание, идёт антидребезг
    BUTTONS_DRIVER_STATE_WAIT_SECOND_CLICK, //!< Ожидание второго нажатия после первого короткого
    BUTTONS_DRIVER_STATE_LONG_FIRED,        //!< Длинное нажатие сгенерировано, ждём отпускания или автоповтора
    BUTTONS_DRIVER_STATE_WAIT_RELEASE       //!< Ожидание отпускания после выдачи события
} ButtonsDriverState;

//! \brief Тип события кнопки
typedef enum
{
    BUTTONS_DRIVER_EVENT_NONE = 0,       //!< Событий нет
    BUTTONS_DRIVER_EVENT_SHORT,          //!< Одиночное короткое нажатие
    BUTTONS_DRIVER_EVENT_DOUBLE,         //!< Двойное короткое нажатие
    BUTTONS_DRIVER_EVENT_LONG,           //!< Длинное нажатие
    BUTTONS_DRIVER_EVENT_REPEAT,         //!< Автоповтор при удержании
    BUTTONS_DRIVER_EVENT_HOLD_START,     //!< Кнопка нажата (после антидребезга)
    BUTTONS_DRIVER_EVENT_HOLD_END        //!< Кнопка отпущена (после антидребезга)
} ButtonsDriverEvent;

//! Маски разрешённых событий (битовые флаги)
#define BUTTONS_DRIVER_ENABLE_SHORT         0x01u   //!< Разрешить одиночное короткое нажатие (всегда включено)
#define BUTTONS_DRIVER_ENABLE_DOUBLE        0x02u   //!< Разрешить двойное короткое нажатие
#define BUTTONS_DRIVER_ENABLE_LONG          0x04u   //!< Разрешить длинное нажатие
#define BUTTONS_DRIVER_ENABLE_REPEAT        0x08u   //!< Разрешить автоповтор при удержании
#define BUTTONS_DRIVER_ENABLE_HOLD_TRACK    0x10u   //!< Разрешить события начала/конца удержания

//! Количество бит, отводимых на событие одной кнопки в упакованном поле
#define BUTTONS_DRIVER_EVENT_BITS            3u

//! Маска события одной кнопки в упакованном поле
#define BUTTONS_DRIVER_EVENT_MASK            0x07u

//! Сдвиг для доступа к событию кнопки с индексом buttonId
#define BUTTONS_DRIVER_EVENT_SHIFT(buttonId) ((buttonId) * BUTTONS_DRIVER_EVENT_BITS)

//! \brief Инициализация модуля ButtonsDriver
void ButtonsDriver_Init(void);

//! \brief Обработчик тика, вызывать в прерывании таймера
//! \details Период вызова должен быть BUTTONS_DRIVER_TICK_MICROSECONDS мкс
void ButtonsDriver_ProcessTick(void);

//! \brief Получить событие кнопки, вызывать в задаче
//! \param[in] buttonId Идентификатор кнопки
//! \return Событие кнопки или BUTTONS_DRIVER_EVENT_NONE, если событий нет
ButtonsDriverEvent ButtonsDriver_GetEvent(Buttons buttonId);

//! \brief Установить callback для уведомления задачи из прерывания
//! \param[in] pCallback Указатель на функцию-callback (может быть NULL)
void ButtonsDriver_SetEventCallback(void (*pCallback)(void));

//! \brief Установить маску разрешённых событий
//! \param[in] enabledEvents Маска разрешённых событий (комбинация BUTTONS_DRIVER_ENABLE_*)
void ButtonsDriver_SetEnabledEvents(uint8_t enabledEvents);

//! \brief Получить текущую маску разрешённых событий
//! \return Маска разрешённых событий (комбинация BUTTONS_DRIVER_ENABLE_*)
uint8_t ButtonsDriver_GetEnabledEvents(void);

//! \brief Получить время последнего удержания кнопки в миллисекундах
//! \details Возвращает длительность последнего завершённого удержания.
//!          Значение обновляется в момент подтверждённого отпускания кнопки.
//!          Антидребезг на обоих фронтах сокращается при вычитании,
//!          поэтому значение уже корректно и не требует коррекции.
//! \param[in] buttonId Идентификатор кнопки
//! \return Время удержания в миллисекундах (0, если не было удержания)
uint32_t ButtonsDriver_GetLastHoldDurationMs(Buttons buttonId);

//! \brief Получить момент последнего нажатия кнопки в миллисекундах
//! \details Возвращает момент времени, когда FSM подтвердила нажатие кнопки
//!          (после антидребезга). Значение обновляется при каждом переходе
//!          FSM в состояние PRESSED. Если кнопка удерживается сейчас —
//!          значение соответствует началу текущего удержания. Если кнопка
//!          отпущена — соответствует последнему завершённому нажатию.
//!
//!          Семантика «с антидребезгом»: момент физического замыкания
//!          наступает на BUTTONS_DRIVER_DEBOUNCE_TICKS * 100 мкс (= 5 мс)
//!          раньше возвращаемого значения.
//!
//!          Когда вычитать антидребезг:
//!          - НЕ НУЖНО, если сравниваете с другой меткой от драйвера кнопок
//!            (интервал между нажатиями, длительность удержания, chord).
//!            Обе метки содержат одинаковую задержку, она сокращается.
//!          - НУЖНО, если сравниваете с внешним событием (UserTimer в момент
//!            запуска зуммера, зажигания LED, старта раунда). Внешнее событие
//!            фиксируется без задержки, поэтому разница содержит +5 мс.
//! \param[in] buttonId Идентификатор кнопки
//! \return Момент нажатия в миллисекундах с момента запуска программы
uint32_t ButtonsDriver_GetPressTimestampMs(Buttons buttonId);

//! \brief Получить момент последнего отпускания кнопки в миллисекундах
//! \details Возвращает момент времени, когда FSM подтвердила отпускание
//!          кнопки (после антидребезга). Значение обновляется при переходе
//!          FSM из PRESSED/LONG_FIRED в RELEASED.
//!
//!          ВАЖНО: если кнопка нажата прямо сейчас, значение соответствует
//!          ПРЕДЫДУЩЕМУ отпусканию, а не текущему состоянию. Не используйте
//!          его для проверки «отпущена ли кнопка сейчас» — для этого есть
//!          ButtonsDriver_GetState и ButtonsDriver_GetPressedMask.
//!
//!          Семантика «с антидребезгом»: момент физического размыкания
//!          наступает на BUTTONS_DRIVER_DEBOUNCE_TICKS * 100 мкс (= 5 мс)
//!          раньше возвращаемого значения.
//!
//!          Когда вычитать антидребезг — аналогично GetPressTimestampMs:
//!          не нужно при сравнении с другими метками драйвера, нужно при
//!          сравнении с внешними событиями.
//! \param[in] buttonId Идентификатор кнопки
//! \return Момент отпускания в миллисекундах с момента запуска программы
uint32_t ButtonsDriver_GetReleaseTimestampMs(Buttons buttonId);

//! \brief Получить текущее состояние конечного автомата кнопки
//! \param[in] buttonId Идентификатор кнопки
//! \return Состояние конечного автомата кнопки
ButtonsDriverState ButtonsDriver_GetState(Buttons buttonId);

//! \brief Получить маску кнопок, которые считаются нажатыми (по FSM)
//! \details Кнопка считается нажатой, если FSM находится в состоянии
//!          BUTTONS_DRIVER_STATE_PRESSED или BUTTONS_DRIVER_STATE_LONG_FIRED.
//!          Маска согласованно читается для всех кнопок.
//! \return Битовая маска нажатых кнопок (1 = нажата)
uint8_t ButtonsDriver_GetPressedMask(void);

//! \brief Сбросить состояние одной кнопки
//! \param[in] buttonId Идентификатор кнопки
void ButtonsDriver_ResetButton(Buttons buttonId);

//! \brief Сбросить состояние всех кнопок
void ButtonsDriver_ResetAll(void);

#ifdef __cplusplus
}
#endif

#endif // INC_BUTTONS_DRIVER_H_
