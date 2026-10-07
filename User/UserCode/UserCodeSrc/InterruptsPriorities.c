#include "stm32f1xx_hal.h"

#include "FreeRTOSConfig.h"

#include "InterruptsPriorities.h"

// Общее правило установки приоритетов прерываний.
// При использовании прерываний периферии и FreeRTOS
// нужно настроить приоритеты прерываний. По умолчанию
// приоритеты всех прерываний максимальны и равны 0
// (при генерации кода в STM32CubeMx). При использовании
// 4 бит для настройки приоритетов прерываний (16 значений)
// для использования FromISR-функций можно задать значения
// приоритетов прерываний в диапазоне
// от configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY
// до configLIBRARY_LOWEST_INTERRUPT_PRIORITY.
// Чем больше значение приоритета численно,
// тем оно имеет более низкий приоритет.

//! Минимально допустимый приоритет для прерываний с FromISR-функциями
#define INTERRUPT_PRIORITY_FREERTOS_MIN     configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY

//! Приоритет прерываний таймера 2
#define TIM2_INTERRUPTS_PRIORITY            (INTERRUPT_PRIORITY_FREERTOS_MIN + 2)

//! \brief Установка приоритетов прерываний для корректной работы FreeRTOS
void InterruptsPriorities_SetPriorities(void)
{
    // По умолчанию установлено NVIC_PRIORITYGROUP_4,
    // добавлено для подстраховки
    HAL_NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);

    // Установка приоритета прерываний таймера 2
    HAL_NVIC_SetPriority(TIM2_IRQn, TIM2_INTERRUPTS_PRIORITY, 0);
}
