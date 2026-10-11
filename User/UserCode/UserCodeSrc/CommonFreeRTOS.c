#include "ButtonsProcessing.h"
#include "BuzzersControl.h"
#include "CommonFreeRTOS.h"
#include "Lcd1602Control.h"
#include "LedsControl.h"
#include "LedsEffectsControl.h"

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

    // TODO: Временно отключено
    // Создание задачи тестирования световых эффектов
//    tasksCreationStatus = LedsEffectsControl_CreateTask();
//
//    if (pdFALSE == tasksCreationStatus)
//    {
//        return pdFALSE;
//    }

    // TODO: Временно отключено
//    // Создание задачи управления LCD1602
//    tasksCreationStatus = Lcd1602Control_CreateTask();
//
//    if (pdFALSE == tasksCreationStatus)
//    {
//        return pdFALSE;
//    }

    // TODO: Временно отключено
//    // Создание задачи управления светодиодами
//    tasksCreationStatus = LedsControl_CreateTask();
//
//    if (pdFALSE == tasksCreationStatus)
//    {
//        return pdFALSE;
//    }

    // TODO: Временно отключено
//    // Создание задачи тестирования зуммеров
//    tasksCreationStatus = BuzzersControl_CreateTask();
//
//    if (pdFALSE == tasksCreationStatus)
//    {
//        return pdFALSE;
//    }

    return tasksCreationStatus;
}
