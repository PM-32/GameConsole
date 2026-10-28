#include "SyncPeripheryControl.h"

#include "FreeRTOS.h"
#include "task.h"

#include "ButtonsDriver.h"
#include "LedsDriver.h"
#include "LedsEffects.h"
#include "SyncPeriphery.h"

//! Приоритет задачи тестирования
#define SYNC_PERIPHERY_CONTROL_TASK_PRIORITY    (tskIDLE_PRIORITY + 2u)

//! Размер стека задачи тестирования, слов
#define SYNC_PERIPHERY_CONTROL_TASK_STACK_SIZE  128u

//! Яркость LED в тестовом режиме, %
#define SYNC_PERIPHERY_CONTROL_BRIGHTNESS       3u

//! Период задачи тестирования, мс
#define SYNC_PERIPHERY_CONTROL_TASK_PERIOD_MS   SYNC_PERIPHERY_UPDATE_PERIOD_MS

//! \brief Задача тестирования SyncPeriphery
//! \param[in] pParameters Параметры задачи (не используются)
//! \note LedsDriver_Show() вызывается только при наличии активности
//!       (активная вспышка SyncPeriphery или активный эффект LedsEffects),
//!       а также один тик после окончания активности — чтобы отправить
//!       нули на ленту и остановить DMA. В остальное время DIN
//!       удерживается в низком уровне, без фантомных импульсов.
static void SyncPeripheryControl_Task(void *pParameters)
{
    (void)pParameters;

    TickType_t lastWakeTime     = xTaskGetTickCount();
    uint8_t    previousActivity = 0u;

    for (;;)
    {
        // Опрос всех кнопок. При коротком нажатии вызываем клик.
        for (uint8_t buttonIndex = 0u; buttonIndex < BUTTONS_QUANTITY; buttonIndex++)
        {
            ButtonsDriverEvent event = ButtonsDriver_GetEvent((Buttons)buttonIndex);

            if (BUTTONS_DRIVER_EVENT_SHORT == event)
            {
                SyncPeriphery_Click();
            }
        }

        // Обновление фонового эффекта и игровых вспышек.
        LedsEffects_Update();
        SyncPeriphery_Update();

        // Определяем, есть ли что-то, что требует отправки на ленту:
        // активные вспышки SyncPeriphery или незавершённый эффект LedsEffects.
        uint8_t activity = 0u;

        if (SyncPeriphery_HasActiveFlashes() != 0u)
        {
            activity = 1u;
        }

        if (LedsEffects_IsFinished() == 0u)
        {
            activity = 1u;
        }

        // Вызываем Show, если активность есть сейчас или была в прошлом тике.
        // Второе условие нужно, чтобы гарантированно отправить нули на ленту
        // в первом тике после затухания последней вспышки и остановить DMA.
        // Между кликами Show не вызывается — DIN остаётся в низком уровне.
        if ((0u != activity) || (0u != previousActivity))
        {
            LedsDriver_Show();
        }

        previousActivity = activity;

        vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(SYNC_PERIPHERY_CONTROL_TASK_PERIOD_MS));
    }
}

//! \brief Создание задачи тестирования SyncPeriphery
//! \return pdPASS при успешном создании задачи, иначе pdFAIL
BaseType_t SyncPeripheryControl_CreateTask(void)
{
    // Установка яркости для тестового режима
    LedsDriver_SetBrightness(SYNC_PERIPHERY_CONTROL_BRIGHTNESS);

    BaseType_t result = xTaskCreate(SyncPeripheryControl_Task,
                                    "SyncPeriphCtrl",
                                    SYNC_PERIPHERY_CONTROL_TASK_STACK_SIZE,
                                    NULL,
                                    SYNC_PERIPHERY_CONTROL_TASK_PRIORITY,
                                    NULL);

    return result;
}
