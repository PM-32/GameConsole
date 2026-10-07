#include "ButtonsProcessing.h"
#include "CommonFreeRTOS.h"

//! \brief Создание задач FreeRTOS
//! \return Статус создания задач FreeRTOS
BaseType_t CommonFreeRTOS_CreateTasks(void)
{
    // Статус создания задач FreeRTOS
    BaseType_t tasksCreationStatus = pdFALSE;

    // Создание задачи обработки кнопок
    tasksCreationStatus = ButtonsProcessing_CreateTask();

    return tasksCreationStatus;
}
