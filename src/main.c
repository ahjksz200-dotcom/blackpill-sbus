#include "main.h"
#include <string.h>

#define CE_HIGH() HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET)
#define CE_LOW()  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET)
#define CSN_HIGH() HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_SET)
#define CSN_LOW()  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_RESET)

extern SPI_HandleTypeDef hspi1;
extern UART_HandleTypeDef huart1;

uint8_t hopTable[] = {90,102,115,97,120};
uint8_t hopIndex = 0;
uint8_t hopCounter = 0;

uint32_t lastPacketTime = 0;

typedef struct {
    uint8_t header;
    uint16_t ch[6];
    uint8_t crc;
} Packet;

Packet rxData;

uint16_t channels[16] = {1500,1500,1000,1500,1000,1000};

uint8_t sbusFrame[25];

uint8_t simpleCRC(uint8_t *buf, int len)
{
    uint8_t c = 0;
    for(int i=0;i<len;i++)
        c ^= buf[i];
    return c;
}

/* ===== SBUS BUILD ===== */

void buildSBUS()
{
    uint16_t sbusCh[16];

    for(int i=0;i<6;i++)
        sbusCh[i] = (channels[i] - 1000) * 2047 / 1000;

    sbusFrame[0] = 0x0F;

    sbusFrame[1]  = (sbusCh[0] & 0xFF);
    sbusFrame[2]  = (sbusCh[0] >> 8) | ((sbusCh[1] & 0x07) << 3);
    sbusFrame[3]  = (sbusCh[1] >> 5) | ((sbusCh[2] & 0x3F) << 6);
    sbusFrame[4]  = (sbusCh[2] >> 2);
    sbusFrame[5]  = (sbusCh[2] >> 10) | ((sbusCh[3] & 0x1FF) << 1);
    sbusFrame[6]  = (sbusCh[3] >> 7) | ((sbusCh[4] & 0x0F) << 4);
    sbusFrame[7]  = (sbusCh[4] >> 4) | ((sbusCh[5] & 0x01) << 7);
    sbusFrame[8]  = (sbusCh[5] >> 1);
    sbusFrame[9]  = (sbusCh[5] >> 9);

    for(int i=10;i<23;i++)
        sbusFrame[i] = 0;

    sbusFrame[23] = 0x00; // flags
    sbusFrame[24] = 0x00;
}

/* ===== FAILSAFE ===== */

void checkFailsafe()
{
    if(HAL_GetTick() - lastPacketTime > 100)
    {
        channels[2] = 1000; // throttle low
        sbusFrame[23] |= (1 << 3); // failsafe flag
    }
}

/* ===== MAIN LOOP ===== */

void loopRX()
{
    if(nRF24_available())   // bạn phải có driver nRF
    {
        nRF24_read(&rxData, sizeof(Packet));

        if(rxData.header == 0xAA &&
           rxData.crc == simpleCRC((uint8_t*)&rxData, sizeof(Packet)-1))
        {
            for(int i=0;i<6;i++)
                channels[i] = rxData.ch[i];

            lastPacketTime = HAL_GetTick();

            hopCounter++;
            if(hopCounter >= 4)
            {
                hopCounter = 0;
                hopIndex++;
                if(hopIndex >= sizeof(hopTable))
                    hopIndex = 0;

                nRF24_setChannel(hopTable[hopIndex]);
            }
        }
    }

    checkFailsafe();
    buildSBUS();
    HAL_UART_Transmit(&huart1, sbusFrame, 25, 10);
}
