#include "stm32f1xx_hal.h"

/* ================= HANDLE ================= */
ADC_HandleTypeDef hadc1;
TIM_HandleTypeDef htim1;

/* ================= BIẾN ================= */
uint32_t adc_raw = 0;
uint32_t adc_filtered = 0;

uint32_t pulse_target = 1000;
uint32_t pulse_current = 1000;

/* ================= PROTOTYPE ================= */
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_ADC1_Init(void);
static void MX_TIM1_Init(void);

/* ================= MAIN ================= */
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
        /* ==== ĐỌC ADC ==== */
        HAL_ADC_Start(&hadc1);
        HAL_ADC_PollForConversion(&hadc1, 10);
        adc_raw = HAL_ADC_GetValue(&hadc1);

        /* ==== LỌC (mượt ga) ==== */
        adc_filtered = (adc_filtered * 3 + adc_raw) / 4;

        /* ==== MAP GA (giới hạn cho EDF) ==== */
        pulse_target = 1000 + (adc_filtered * 600) / 4095; // max ~1600us

        /* Deadzone */
        if (pulse_target < 1050) pulse_target = 1000;

        /* ==== SOFT START ==== */
        if (pulse_current < pulse_target) pulse_current += 1;
        else if (pulse_current > pulse_target) pulse_current -= 3;

        /* ==== XUẤT PWM ==== */
        __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, pulse_current);

        HAL_Delay(2);
    }
}

/* ================= GPIO ================= */
static void MX_GPIO_Init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_AFIO_CLK_ENABLE();
}

/* ================= ADC ================= */
static void MX_ADC1_Init(void)
{
    ADC_ChannelConfTypeDef sConfig = {0};
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_ADC1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    /* PA0 = Analog */
    GPIO_InitStruct.Pin = GPIO_PIN_0;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    hadc1.Instance = ADC1;
    hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
    hadc1.Init.ContinuousConvMode = DISABLE;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc1.Init.NbrOfConversion = 1;

    HAL_ADC_Init(&hadc1);

    sConfig.Channel = ADC_CHANNEL_0; // PA0
    sConfig.Rank = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = ADC_SAMPLETIME_71CYCLES_5;

    HAL_ADC_ConfigChannel(&hadc1, &sConfig);
}

/* ================= PWM ESC ================= */
static void MX_TIM1_Init(void)
{
    TIM_OC_InitTypeDef sConfigOC = {0};
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_TIM1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    /* PA8 = PWM */
    GPIO_InitStruct.Pin = GPIO_PIN_8;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    htim1.Instance = TIM1;
    htim1.Init.Prescaler = 72 - 1;      // 1MHz (1us)
    htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim1.Init.Period = 20000 - 1;      // 20ms (50Hz)
    htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;

    HAL_TIM_PWM_Init(&htim1);

    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = 1000; // min throttle
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;

    HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_1);
}

/* ================= CLOCK ================= */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    /* HSE 8MHz -> 72MHz */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;

    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;

    HAL_RCC_OscConfig(&RCC_OscInitStruct);

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_SYSCLK |
                                 RCC_CLOCKTYPE_HCLK |
                                 RCC_CLOCKTYPE_PCLK1 |
                                 RCC_CLOCKTYPE_PCLK2;

    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;

    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2);
}

/* ================= ERROR ================= */
void Error_Handler(void)
{
    while (1)
    {
    }
}
