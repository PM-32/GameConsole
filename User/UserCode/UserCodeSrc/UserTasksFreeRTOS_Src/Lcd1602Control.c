#include <string.h>

#include "FreeRTOS.h"
#include "task.h"

#include "Hardware.h"
#include "Lcd1602Control.h"
#include "Lcd1602Driver.h"
#include "Lcd1602Strings.h"

//! Период одной фазы теста, мс
#define LCD1602_CONTROL_TASK_PERIOD_MS    3000u

//! Размер стека задачи управления LCD1602, слов
#define LCD1602_CONTROL_TASK_STACK_SIZE   256u

//! Приоритет задачи управления LCD1602
#define LCD1602_CONTROL_TASK_PRIORITY     (tskIDLE_PRIORITY + 1u)

//! Количество фаз теста
#define LCD1602_CONTROL_PHASES_COUNT      2u

//! \brief  Задача управления LCD1602
//! \param[in] pParameters - параметры задачи (не используются)
//! \note   Циклический тест из двух фаз:
//!         1) "ПРИВЕТ" в левом верхнем углу,
//!         2) "Hello" в правом нижнем углу.
//!         Смена фаз - каждые LCD1602_CONTROL_TASK_PERIOD_MS мс.
static void Lcd1602Control_Task(void *pParameters)
{
    (void)pParameters;

    uint8_t asciiLength = 0u;
    uint8_t asciiRightCol = 0u;
    uint8_t phase = 0u;

    // Расчёт позиции "Hello" для правого нижнего угла
    asciiLength = (uint8_t)strlen(LCD1602_STR_HELLO_ASCII);

    if (asciiLength <= LCD1602_COLS)
    {
        asciiRightCol = LCD1602_COLS - asciiLength;
    }
    else
    {
        asciiRightCol = 0u;
    }

    while (1)
    {
        switch (phase)
        {
            case 0u:
            {
                // Фаза 1: "ПРИВЕТ" в левом верхнем углу
                Lcd1602Driver_Clear();
                Lcd1602Driver_PrintCyrillicAt(0u, 0u, LCD1602_STR_HELLO);

                break;
            }

            case 1u:
            {
                // Фаза 2: "Hello" в правом нижнем углу
                Lcd1602Driver_Clear();
                Lcd1602Driver_PrintAt(1u, asciiRightCol, LCD1602_STR_HELLO_ASCII);

                break;
            }

            default:
            {
                // Недостижимо, защита от неверного состояния
                phase = 0u;
                continue;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(LCD1602_CONTROL_TASK_PERIOD_MS));

        // Переход к следующей фазе
        phase++;

        if (phase >= LCD1602_CONTROL_PHASES_COUNT)
        {
            phase = 0u;
        }
    }
}

//! \brief  Создание задачи управления LCD1602
//! \return Статус создания задачи FreeRTOS
BaseType_t Lcd1602Control_CreateTask(void)
{
    BaseType_t taskCreationStatus = pdFALSE;

    taskCreationStatus = xTaskCreate(Lcd1602Control_Task,
                                     "Lcd1602Control",
                                     LCD1602_CONTROL_TASK_STACK_SIZE,
                                     NULL,
                                     LCD1602_CONTROL_TASK_PRIORITY,
                                     NULL);

    return taskCreationStatus;
}
