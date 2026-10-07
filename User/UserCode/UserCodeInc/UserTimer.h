#ifndef INC_USER_TIMER_H_
#define INC_USER_TIMER_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>

//! Количество периодов таймера в одной миллисекунде (1 мс = 100 мкс * 10)
#define USER_TIMER_PERIODS_IN_1_MS     10u

//! \brief Инкремент счётчика времени работы программы
//! \details Вызывать в прерывании таймера с периодом 100 мкс
void UserTimer_IncrementTimeCounter(void);

//! \brief Получить счётчик времени работы программы в тиках таймера
//! \return Счётчик времени в тиках (1 тик = 100 мкс)
uint32_t UserTimer_GetTimeCounter(void);

//! \brief Получить время работы программы в миллисекундах
//! \return Время работы программы в миллисекундах
uint32_t UserTimer_GetTimeCounterInMs(void);

#ifdef __cplusplus
}
#endif

#endif // INC_USER_TIMER_H_
