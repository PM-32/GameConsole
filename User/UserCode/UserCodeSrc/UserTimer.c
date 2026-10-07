#include "UserTimer.h"

//! Счётчик времени работы программы в тиках таймера
static volatile uint32_t userTimerTimeCounter = 0u;

//! \brief Инкремент счётчика времени работы программы
//! \details Вызывать в прерывании таймера с периодом 100 мкс
void UserTimer_IncrementTimeCounter(void)
{
    userTimerTimeCounter++;
}

//! \brief Получить счётчик времени работы программы в тиках таймера
//! \return Счётчик времени в тиках (1 тик = 100 мкс)
uint32_t UserTimer_GetTimeCounter(void)
{
    return userTimerTimeCounter;
}

//! \brief Получить время работы программы в миллисекундах
//! \return Время работы программы в миллисекундах
uint32_t UserTimer_GetTimeCounterInMs(void)
{
    return (uint32_t)(userTimerTimeCounter / USER_TIMER_PERIODS_IN_1_MS);
}
