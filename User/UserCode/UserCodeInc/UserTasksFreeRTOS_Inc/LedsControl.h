#ifndef INC_LEDS_CONTROL_H_
#define INC_LEDS_CONTROL_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "FreeRTOS.h"
#include "task.h"

//! \brief Базовые цвета для проверки и служебных индикаций
//! \details Значения компонент соответствуют стандартным RGB-цветам
typedef enum
{
    LEDS_CONTROL_COLOR_BLACK = 0,    //!< Чёрный (выключено)
    LEDS_CONTROL_COLOR_RED,          //!< Красный
    LEDS_CONTROL_COLOR_GREEN,        //!< Зелёный
    LEDS_CONTROL_COLOR_BLUE,         //!< Синий
    LEDS_CONTROL_COLOR_YELLOW,       //!< Жёлтый
    LEDS_CONTROL_COLOR_CYAN,         //!< Голубой
    LEDS_CONTROL_COLOR_MAGENTA,      //!< Пурпурный
    LEDS_CONTROL_COLORS_QUANTITY     //!< Количество базовых цветов (маркер)
} LedsControlColor;

//! \brief Создание задачи управления светодиодами
//! \return Результат (статус) создания задачи
BaseType_t LedsControl_CreateTask(void);

#ifdef __cplusplus
}
#endif

#endif // INC_LEDS_CONTROL_H_
