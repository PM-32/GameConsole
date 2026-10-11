#ifndef INC_LCD1602_CONTROL_H_
#define INC_LCD1602_CONTROL_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "FreeRTOS.h"
#include "task.h"

//! \brief  Создание задачи управления LCD1602
//! \return Статус создания задачи FreeRTOS
BaseType_t Lcd1602Control_CreateTask(void);

#ifdef __cplusplus
}
#endif

#endif // INC_LCD1602_CONTROL_H_
