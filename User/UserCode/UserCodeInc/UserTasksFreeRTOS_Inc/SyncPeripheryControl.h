#ifndef INC_SYNC_PERIPHERY_CONTROL_H_
#define INC_SYNC_PERIPHERY_CONTROL_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "FreeRTOS.h"

//! \brief Создание задачи тестирования SyncPeriphery
//! \return pdPASS при успешном создании задачи, иначе pdFAIL
//! \note Задача опрашивает кнопки через ButtonsDriver_GetEvent.
//!       При BUTTONS_DRIVER_EVENT_SHORT на любой кнопке вызывает
//!       SyncPeriphery_Click(). Каждый тик (20 мс) обновляет
//!       LedsEffects_Update() → SyncPeriphery_Update().
//!       LedsDriver_Show() вызывается ТОЛЬКО если есть активная
//!       вспышка или эффект, либо если активность была в прошлом тике
//!       (чтобы после затухания один раз отправить нули на ленту).
BaseType_t SyncPeripheryControl_CreateTask(void);

#ifdef __cplusplus
}
#endif

#endif // INC_SYNC_PERIPHERY_CONTROL_H_
