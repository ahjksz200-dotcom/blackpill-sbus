#include "stm32f1xx_hal.h"
#include <string.h>

/* ========= PIN ========= */
#define CE_PIN GPIO_PIN_0
#define CE_PORT GPIOB

#define CSN_PIN GPIO_PIN_1
#define CSN_PORT GPIOB

SPI_HandleTypeDef hspi1;
UART_HandleTypeDef huart1;

/* ========= FHSS ========= */
uint8_t hopTable[] = {90,102,115,97,120};
uint8_t hopIndex = 0;
uint8_t hopCounter = 0;

/* ========= DATA ========= */
typedef struct {
    uint8_t header;
    uint16_t ch[6];
    uint8_t crc;
} Packet;

Packet rxData;

uint16_t channels[6] = {1500,1500,1000,1500,1000,1000};

uint32_t lastPacketTime = 0;

/* ========= LOW LEVEL ========= */

void CE_HIGH() { HAL_GPIO_WritePin(CE_PORT, CE_PIN, GPIO_PIN_SET); }
void CE_LOW()  { HAL_GPIO_WritePin(CE_PORT, CE_PIN, GPIO_PIN_RESET); }

void CSN_HIGH(){ HAL_GPIO_WritePin(CSN_PORT, CSN_PIN, GPIO_PIN_SET); }
void CSN_LOW() { HAL_GPIO_WritePin(CSN_PORT, CSN_PIN, GPIO_PIN_RESET); }

uint8_t SPI_RW(uint8_t data)
{
    uint8_t rx;
    HAL_SPI_TransmitReceive(&hspi1, &data, &rx, 1, 10);
    return rx;
}

/* ========= CRC ========= */

uint8_t simpleCRC(uint8_t *buf, int len)
{
    uint8_t c = 0;
    for(int i=0;i<len;i++)
        c ^= buf[i];
    return c;
}

/* ========= SBUS ========= */

uint8_t sbusFrame[25];

void buildSBUS()
{
    uint16_t sbus[6];

    for(int i=0;i<6;i++)
        sbus[i] = (channels[i] - 1000) * 2047 / 1000;

    memset(sbusFrame,0,25);

    sbusFrame[0] = 0x0F;

    sbusFrame[1] = sbus[0] & 0xFF;
    sbusFrame[2] = (sbus[0] >> 8) | ((sbus[1] & 0x07) << 3);
    sbusFrame[3] = (sbus[1] >> 5) | ((sbus[2] & 0x3F) << 6);
    sbusFrame[4] = (sbus[2] >> 2);
    sbusFrame[5] = (sbus[2] >> 10) | ((sbus[3] & 0x1FF) << 1);
    sbusFrame[6] = (sbus[3] >> 7) | ((sbus[4] & 0x0F) << 4);
    sbusFrame[7] = (sbus[4] >> 4) | ((sbus[5] & 0x01) << 7);
    sbusFrame[8] = (sbus[5] >> 1);
    sbusFrame[9] = (sbus[5] >> 9);
}

/* ========= CLOCK ========= */

void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI_DIV2;
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL16;
    HAL_RCC_OscConfig(&RCC_OscInitStruct);

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_SYSCLK|
                                  RCC_CLOCKTYPE_PCLK1|
                                  RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2);
}

/* ========= INIT ========= */

void MX_GPIO_Init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Pin = CE_PIN | CSN_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

void MX_SPI1_Init(void)
{
    __HAL_RCC_SPI1_CLK_ENABLE();

    hspi1.Instance = SPI1;
    hspi1.Init.Mode = SPI_MODE_MASTER;
    hspi1.Init.Direction = SPI_DIRECTION_2LINES;
    hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
    hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
    hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
    hspi1.Init.NSS = SPI_NSS_SOFT;
    hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;
    hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;

    HAL_SPI_Init(&hspi1);
}

void MX_USART1_UART_Init(void)
{
    __HAL_RCC_USART1_CLK_ENABLE();

    huart1.Instance = USART1;
    huart1.Init.BaudRate = 100000;
    huart1.Init.WordLength = UART_WORDLENGTH_9B;
    huart1.Init.StopBits = UART_STOPBITS_2;
    huart1.Init.Parity = UART_PARITY_EVEN;
    huart1.Init.Mode = UART_MODE_TX;

    HAL_UART_Init(&huart1);
}

/* ========= MAIN ========= */

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_SPI1_Init();
    MX_USART1_UART_Init();

    while(1)
    {
        /* Ở đây bạn sẽ thêm nRF read sau */
        
        buildSBUS();
        HAL_UART_Transmit(&huart1, sbusFrame, 25, 10);
        HAL_Delay(14);
    }
}
