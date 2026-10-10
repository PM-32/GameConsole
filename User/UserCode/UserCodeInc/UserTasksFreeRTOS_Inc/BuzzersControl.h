#ifndef INC_BUZZERS_CONTROL_H_
#define INC_BUZZERS_CONTROL_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "FreeRTOS.h"
#include "task.h"

//! \brief Создание задачи тестирования зуммеров (F-28)
//! \return Результат (статус) создания задачи
BaseType_t BuzzersControl_CreateTask(void);

#ifdef __cplusplus
}
#endif

#endif // INC_BUZZERS_CONTROL_H_
