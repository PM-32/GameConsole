#include "FreeRTOS.h"
#include "task.h"

//! \brief Хук (обработчик) переполнения стека
//! \param[in] xTask Дескриптор задачи, у которой переполнился стек
//! \param[in] pcTaskName Имя задачи, у которой переполнился стек
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;
    (void)pcTaskName;

    taskDISABLE_INTERRUPTS();

    for (;;)
    {
    }
}
