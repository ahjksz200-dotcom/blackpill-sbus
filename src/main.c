#include "stm32f4xx_hal.h"

/* ================= CONFIG ================= */

#define PWM_CHANNELS 6
#define SBUS_CHANNELS 16

UART_HandleTypeDef huart1;
TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim4;

/* ================= GLOBAL ================= */

uint16_t pwm_value[PWM_CHANNELS] = {1500,1500,1500,1500,1500,1500};
uint16_t sbus_channel[SBUS_CHANNELS];
uint8_t sbus_frame[25];

/* ================= SBUS ================= */

uint16_t convert_to_sbus(uint16_t pwm)
{
    if(pwm < 1000) pwm = 1000;
    if(pwm > 2000) pwm = 2000;

    return (uint16_t)((pwm - 1000) * 1639 / 1000 + 172);
}

void sbus_build_frame()
{
    sbus_frame[0] = 0x0F;

    sbus_frame[1]  = (sbus_channel[0] & 0x07FF);
    sbus_frame[2]  = (sbus_channel[0] >> 8) | ((sbus_channel[1] & 0x07FF) << 3);
    sbus_frame[3]  = (sbus_channel[1] >> 5) | ((sbus_channel[2] & 0x07FF) << 6);
    sbus_frame[4]  = (sbus_channel[2] >> 2);
    sbus_frame[5]  = (sbus_channel[2] >> 10) | ((sbus_channel[3] & 0x07FF) << 1);
    sbus_frame[6]  = (sbus_channel[3] >> 7) | ((sbus_channel[4] & 0x07FF) << 4);
    sbus_frame[7]  = (sbus_channel[4] >> 4) | ((sbus_channel[5] & 0x07FF) << 7);
    sbus_frame[8]  = (sbus_channel[5] >> 1);
    sbus_frame[9]  = (sbus_channel[5] >> 9);

    for(int i=10;i<23;i++)
        sbus_frame[i]=0;

    sbus_frame[23] = 0x00;
    sbus_frame[24] = 0x00;
}

/* ================= TIMER CALLBACK ================= */

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if(htim->Instance == TIM4)
    {
        for(int i=0;i<6;i++)
            sbus_channel[i] = convert_to_sbus(pwm_value[i]);

        for(int i=6;i<16;i++)
            sbus_channel[i] = 1024;

        sbus_build_frame();

        HAL_UART_Transmit(&huart1, sbus_frame, 25, 5);
    }
}

/* ================= INIT ================= */

void SystemClock_Config(void);
static void MX_USART1_UART_Init(void);
static void MX_TIM4_Init(void);

int main(void)
{
    HAL_Init();
    SystemClock_Config();

    MX_USART1_UART_Init();
    MX_TIM4_Init();

    HAL_TIM_Base_Start_IT(&htim4);

    while (1)
    {
    }
}

/* ================= UART SBUS ================= */

static void MX_USART1_UART_Init(void)
{
    __HAL_RCC_USART1_CLK_ENABLE();

    huart1.Instance = USART1;
    huart1.Init.BaudRate = 100000;
    huart1.Init.WordLength = UART_WORDLENGTH_9B;
    huart1.Init.StopBits = UART_STOPBITS_2;
    huart1.Init.Parity = UART_PARITY_EVEN;
    huart1.Init.Mode = UART_MODE_TX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;

    HAL_UART_Init(&huart1);

    /* TX Inversion */
    SET_BIT(huart1.Instance->CR2, USART_CR2_TXINV);
}

/* ================= TIMER 7ms ================= */

static void MX_TIM4_Init(void)
{
    __HAL_RCC_TIM4_CLK_ENABLE();

    htim4.Instance = TIM4;
    htim4.Init.Prescaler = 83;
    htim4.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim4.Init.Period = 7000-1;
    htim4.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;

    HAL_TIM_Base_Init(&htim4);
    HAL_NVIC_SetPriority(TIM4_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(TIM4_IRQn);
}

void TIM4_IRQHandler(void)
{
    HAL_TIM_IRQHandler(&htim4);
}

/* ================= CLOCK ================= */

void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM = 8;
    RCC_OscInitStruct.PLL.PLLN = 336;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
    RCC_OscInitStruct.PLL.PLLQ = 7;

    HAL_RCC_OscConfig(&RCC_OscInitStruct);

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_SYSCLK|RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2);
}
