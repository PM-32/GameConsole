#ifndef INC_BUTTONS_PROCESSING_H_
#define INC_BUTTONS_PROCESSING_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "FreeRTOS.h"
#include "task.h"

//! \brief Создание задачи для обработки нажатий на кнопки
//! \return Результат (статус) создания задачи
BaseType_t ButtonsProcessing_CreateTask(void);

#ifdef __cplusplus
}
#endif

#endif // INC_BUTTONS_PROCESSING_H_
