#include "Hardware.h"
#include "LedsDriver.h"

//! \brief Обработчик прерывания таймера по завершению генериции ШИМ-сигнала
void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim)
{
    if (TIM_PWM_GENERATION == htim->Instance)
    {
        // Callback завершения передачи DMA
        LedsDriver_TransferCompleteCallback();
    }
}
