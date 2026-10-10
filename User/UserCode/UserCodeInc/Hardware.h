#ifndef INC_HARDWARE_H_
#define INC_HARDWARE_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "tim.h"

#define TIM_BUZZERS                 TIM1            //!< Таймер для генерации ШИМ-сигнала управления зуммерами
#define TIM_BUZZERS_HTIM            htim1           //!< Таймер для генерации ШИМ-сигнала управления зуммерами (через htim)
#define LARGE_BUZZER_PWM_CHANNEL    TIM_CHANNEL_1   //!< Канал таймера для большого зуммера (PA8)
#define SMALL_BUZZER_PWM_CHANNEL    TIM_CHANNEL_2   //!< Канал таймера для маленького зуммера (PA9)

#define TIM_INTERVAL        		TIM2        	//!< Таймер для замера временных интервалов 100 мкс
#define TIM_INTERVAL_HTIM   		htim2       	//!< Таймер для замера временных интервалов 100 мкс (через htim)

#define TIM_PWM_GENERATION			TIM4			//!< Таймер для генерации ШИМ-сигнала управления светодиодами WS2812B
#define TIM_PWM_GENERATION_HTIM		htim4			//!< Таймер для генерации ШИМ-сигнала управления светодиодами WS2812B (через htim)
#define TIM_PWM_GENERATION_CHANNEL	TIM_CHANNEL_3	//!< Канал таймера для генерации ШИМ-сигнала управления светодиодами WS2812B

#ifdef __cplusplus
}
#endif

#endif // INC_HARDWARE_H_
