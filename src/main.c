#include "stm32f1xx_hal.h"

ADC_HandleTypeDef hadc1;
TIM_HandleTypeDef htim1;

uint32_t adc_raw = 0;
uint32_t pulse = 1000;

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_ADC1_Init(void);
static void MX_TIM1_Init(void);

int main(void)
{
    HAL_Init();
    SystemClock_Config();

    MX_GPIO_Init();
    MX_ADC1_Init();
    MX_TIM1_Init();

    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);

    while (1)
    {
        HAL_ADC_Start(&hadc1);
        HAL_ADC_PollForConversion(&hadc1, 10);
        adc_raw = HAL_ADC_GetValue(&hadc1);

        // Map 0-4095 -> 1000-2000 us
        pulse = 1000 + (adc_raw * 1000) / 4095;

        // Chống rung nhẹ
        if (pulse < 1000) pulse = 1000;
        if (pulse > 2000) pulse = 2000;

        __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, pulse);

        HAL_Delay(5);
    }
}
