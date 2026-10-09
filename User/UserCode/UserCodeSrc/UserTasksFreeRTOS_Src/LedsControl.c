#include "FreeRTOS.h"
#include "task.h"

#include "LedsControl.h"
#include "LedsDriver.h"

#define LEDS_CONTROL_TASK_STACK_SIZE        (configMINIMAL_STACK_SIZE)
#define LEDS_CONTROL_TASK_PRIORITY          (tskIDLE_PRIORITY + 1u)

#define LEDS_CONTROL_TRANSFER_TIMEOUT_MS    300u    //!< Таймаут ожидания передачи DMA, мс
#define LEDS_CONTROL_COLOR_UPDATE_MS        500u    //!< Период смены базового цвета, мс
#define LEDS_CONTROL_BRIGHTNESS_PERCENT     3u      //!< Яркость ленты для тестового режима, %

//! \brief Таблица базовых цветов (R, G, B)
static const uint8_t ledsControlColors[LEDS_CONTROL_COLORS_QUANTITY][3] =
{
    {   0u,   0u,   0u },    //!< Чёрный
    { 255u,   0u,   0u },    //!< Красный
    {   0u, 255u,   0u },    //!< Зелёный
    {   0u,   0u, 255u },    //!< Синий
    { 255u, 255u,   0u },    //!< Жёлтый
    {   0u, 255u, 255u },    //!< Голубой
    { 255u,   0u, 255u }     //!< Пурпурный
};

static TaskHandle_t ledsControlTaskHandle = NULL;   //!< Хэндл задачи управления светодиодами

//! \brief Callback завершения передачи DMA (вызывается из ISR)
static void LedsControl_OnTransferComplete(void)
{
    BaseType_t higherPriorityTaskWoken = pdFALSE;

    if (ledsControlTaskHandle != NULL)
    {
        vTaskNotifyGiveFromISR(ledsControlTaskHandle, &higherPriorityTaskWoken);
        portYIELD_FROM_ISR(higherPriorityTaskWoken);
    }
}

//! \brief Установка базового цвета на все светодиоды
//! \param[in] colorIndex Индекс базового цвета
static void LedsControl_SetBaseColor(uint8_t colorIndex)
{
    if (colorIndex >= LEDS_CONTROL_COLORS_QUANTITY)
    {
        return;
    }

    uint8_t red   = ledsControlColors[colorIndex][0];
    uint8_t green = ledsControlColors[colorIndex][1];
    uint8_t blue  = ledsControlColors[colorIndex][2];

    LedsDriver_SetAll(red, green, blue);
}

//! \brief Задача управления светодиодами
//! \param[in] pvParameters Параметры задачи (не используются)
static void LedsControl_Task(void *pvParameters)
{
    (void)pvParameters;

    // Сохранение хэндла текущей задачи для callback из ISR
    ledsControlTaskHandle = xTaskGetCurrentTaskHandle();

    // Установка callback завершения передачи
    LedsDriver_SetTransferCompleteCallback(LedsControl_OnTransferComplete);

    // Установка яркости для тестового режима (ограничение тока)
    LedsDriver_SetBrightness(LEDS_CONTROL_BRIGHTNESS_PERCENT);

    uint8_t colorIndex = LEDS_CONTROL_COLOR_RED;

    for (;;)
    {
        // Установка текущего базового цвета на все светодиоды
        LedsControl_SetBaseColor(colorIndex);

        // Запуск передачи на ленту
        LedsDriver_Show();

        // Ожидание завершения передачи (уведомление из ISR)
        ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(LEDS_CONTROL_TRANSFER_TIMEOUT_MS));

        // Переход к следующему базовому цвету по кругу
        colorIndex++;

        if (colorIndex >= LEDS_CONTROL_COLORS_QUANTITY)
        {
            colorIndex = LEDS_CONTROL_COLOR_RED;
        }

        // Пауза перед сменой цвета
        vTaskDelay(pdMS_TO_TICKS(LEDS_CONTROL_COLOR_UPDATE_MS));
    }
}

//! \brief Создание задачи управления светодиодами
//! \return Результат (статус) создания задачи
BaseType_t LedsControl_CreateTask(void)
{
    // Создание задачи FreeRTOS
    BaseType_t xTaskCreationResult = xTaskCreate(LedsControl_Task, "Leds",
                                                 LEDS_CONTROL_TASK_STACK_SIZE, NULL,
                                                 LEDS_CONTROL_TASK_PRIORITY,
                                                 &ledsControlTaskHandle);

    return xTaskCreationResult;
}
