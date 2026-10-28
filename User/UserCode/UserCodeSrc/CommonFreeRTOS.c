#include "ButtonsProcessing.h"
#include "BuzzersControl.h"
#include "CommonFreeRTOS.h"
#include "Lcd1602Control.h"
#include "LedsControl.h"
#include "LedsEffectsControl.h"
#include "SyncPeripheryControl.h"

//! \brief Создание задач FreeRTOS
//! \return Статус создания задач FreeRTOS
//! \note ButtonsProcessing временно отключён.
//!       События кнопок обрабатываются внутри SyncPeripheryControl
//!       (через ButtonsDriver_GetEvent). Это допустимо для теста.
//!       При интеграции игр — либо включить ButtonsProcessing и убрать
//!       опрос кнопок из SyncPeripheryControl, либо оставить одну задачу
//!       обработки. Важно, чтобы события кнопок читала ТОЛЬКО ОДНА
//!       задача (GetEvent атомарно очищает событие).
BaseType_t CommonFreeRTOS_CreateTasks(void)
{
    // Статус создания задач FreeRTOS
    BaseType_t tasksCreationStatus = pdFALSE;

    // Создание задачи обработки кнопок
//    tasksCreationStatus = ButtonsProcessing_CreateTask();
//
//    if (pdFALSE == tasksCreationStatus)
//    {
//        return pdFALSE;
//    }

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

    // Создание задачи тестирования SyncPeriphery
    tasksCreationStatus = SyncPeripheryControl_CreateTask();

    if (pdPASS != tasksCreationStatus)
    {
        return pdFALSE;
    }

    return tasksCreationStatus;
}
