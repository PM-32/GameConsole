#include "ButtonsProcessing.h"
#include "CommonFreeRTOS.h"
#include "LedsControl.h"

//! \brief Создание задач FreeRTOS
//! \return Статус создания задач FreeRTOS
BaseType_t CommonFreeRTOS_CreateTasks(void)
{
    // Статус создания задач FreeRTOS
    BaseType_t tasksCreationStatus = pdFALSE;

    // Создание задачи обработки кнопок
    tasksCreationStatus = ButtonsProcessing_CreateTask();

    if (pdFALSE == tasksCreationStatus)
    {
        return pdFALSE;
    }

    // Создание задачи управления светодиодами
    tasksCreationStatus = LedsControl_CreateTask();

    if (pdFALSE == tasksCreationStatus)
    {
        return pdFALSE;
    }

    return tasksCreationStatus;
}
