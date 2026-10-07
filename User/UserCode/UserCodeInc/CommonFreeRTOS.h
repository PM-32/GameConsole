#ifndef INC_COMMON_FREERTOS_H_
#define INC_COMMON_FREERTOS_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "FreeRTOS.h"
#include "task.h"

//! \brief Создание задач FreeRTOS
//! \return Статус создания задач FreeRTOS
BaseType_t CommonFreeRTOS_CreateTasks(void);

#ifdef __cplusplus
}
#endif

#endif // INC_COMMON_FREERTOS_H_
