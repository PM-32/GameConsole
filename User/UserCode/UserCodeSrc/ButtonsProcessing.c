#include "gpio.h"
#include "main.h"

#include "ButtonsDriver.h"
#include "ButtonsProcessing.h"
#include "UserTimer.h"

static TaskHandle_t buttonsDriverTaskHandle = NULL;		//!< Хендл задачи обработки нажатий на кнопки

//! \brief Callback из прерывания — уведомляет задачу через FreeRTOS
static void ButtonsDriver_OnEvent(void)
{
    BaseType_t higherPriorityTaskWoken = pdFALSE;

    vTaskNotifyGiveFromISR(buttonsDriverTaskHandle, &higherPriorityTaskWoken);

    portYIELD_FROM_ISR(higherPriorityTaskWoken);
}

//! \brief Задача обработки нажатий на кнопки
//! \param[in] pvParameters Параметры задачи (не используются)
static void ButtonsProcessing_Task(void *pvParameters)
{
    (void)pvParameters;

    for (;;)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        for (uint8_t buttonIndex = 0; buttonIndex < BUTTONS_QUANTITY; buttonIndex++)
        {
            ButtonsDriverEvent event = ButtonsDriver_GetEvent((Buttons)buttonIndex);

            switch (event)
            {
                case BUTTONS_DRIVER_EVENT_SHORT:
                {
                    // Обработка короткого нажатия

                	// Тестовая проверка кнопки
					if (0 == buttonIndex)
					{
						HAL_GPIO_TogglePin(GreenLed_GPIO_Port, GreenLed_Pin);
					}

                    break;
                }

                case BUTTONS_DRIVER_EVENT_DOUBLE:
                {
                    // Обработка двойного короткого нажатия

                    break;
                }

                case BUTTONS_DRIVER_EVENT_LONG:
                {
                    // Обработка длинного нажатия

                    break;
                }

                case BUTTONS_DRIVER_EVENT_REPEAT:
                {
                    // Обработка повторного события при долгом удержании
                	// (только при включенном BUTTONS_DRIVER_EVENT_LONG)

                    break;
                }

                case BUTTONS_DRIVER_EVENT_HOLD_START:
                {
                    // Обработка события начала нажатия кнопки

                	// TODO: Код ниже будет использоваться позже
//                	if (0 == buttonIndex)
//					{
//                		// Получение времени начала нажатия на кнопку
//                		uint32_t pressTimeMs = UserTimer_GetTimeCounterInMs();
//					}

                    break;
                }

                case BUTTONS_DRIVER_EVENT_HOLD_END:
                {
                    // Обработка события завершения нажатия кнопки

                	// TODO: Код ниже будет использоваться позже
//                	if (0 == buttonIndex)
//                	{
//                		// Получение времени последнего отпускания кнопки
//						uint32_t holdDurationMs = ButtonsDriver_GetLastHoldDurationMs((Buttons)buttonIndex);
//                	}

                    break;
                }

                default:
                {
                    break;
                }
            }
        }
    }
}

//! \brief Создание задачи для обработки нажатий на кнопки
//! \return Результат (статус) создания задачи
BaseType_t ButtonsProcessing_CreateTask(void)
{
    // Создание задачи FreeRTOS
    BaseType_t xTaskCreationResult = xTaskCreate(ButtonsProcessing_Task, "Buttons",
                                                 128, NULL, 1, &buttonsDriverTaskHandle);

    ButtonsDriver_SetEventCallback(ButtonsDriver_OnEvent);

    return xTaskCreationResult;
}
