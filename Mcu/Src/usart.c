/* usart.c */
#include "usart.h"
#include "odom.h"
extern UART_HandleTypeDef huart1;

static uint8_t rxbuf[256];
static uint16_t head = 0;

uint8_t* usart_get_rx_buf(void) { return rxbuf; }

void usart_parse_loop(void)
{
    uint16_t produced = (256 - __HAL_DMA_GET_COUNTER(huart1.hdmarx)) & 0xFF;
    while (head != produced)
    {
        uint16_t idx = head;
        uint16_t next = (idx + 1) & 0xFF;
        if (rxbuf[idx] == 0xAA && rxbuf[next] == 0x55)
        {
            uint8_t len = rxbuf[(idx + 2) & 0xFF];
            if (len == 10 && remaining(produced, idx) >= 13) // 0xAA 55 Len 10 B 负载 + 1 XOR
            {
                uint8_t *pld = &rxbuf[(idx + 3) & 0xFF];
                uint8_t xor = 0;
                for (int i = 2; i < 12; i++) xor ^= rxbuf[(idx + i) & 0xFF];
                if (xor == rxbuf[(idx + 12) & 0xFF] && pld[0] == 0x01)
                {
                    Speed_t s; 
                    memcpy(&s.vx, &pld[1], 4); // 解出 vx (m/s)
                    memcpy(&s.wz, &pld[5], 4); // 解出 wz (rad/s)
                    odom_set_target(&s);       // 把速度交给运动学模块
                }
                head = (idx + 13) & 0xFF; // 消费整帧
                continue;
            }
        }
        head = (head + 1) & 0xFF;
    }
}

static inline int remaining(uint16_t p, uint16_t h)
{
    int d = p - h; 
    return d < 0 ? d + 256 : d;
}