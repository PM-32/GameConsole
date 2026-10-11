#ifndef INC_LEDS_EFFECTS_CONTROL_H_
#define INC_LEDS_EFFECTS_CONTROL_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "FreeRTOS.h"
#include "task.h"

//! \brief Создание задачи тестирования световых эффектов
//! \return Результат (статус) создания задачи
BaseType_t LedsEffectsControl_CreateTask(void);

#ifdef __cplusplus
}
#endif

#endif // INC_LEDS_EFFECTS_CONTROL_H_
