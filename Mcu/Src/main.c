include "main.h"
#include "usart.h"
#include "can.h"
#include "odom.h"

int main()
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_DMA_Init();
    MX_USART1_UART_Init();
    MX_CAN_Init();
    MX_TIM2_Init();

    odom_init();
    HAL_TIM_Base_Start_IT(&htim2);      // 10 ms 基准
    HAL_UART_Receive_DMA(&huart1, usart_get_rx_buf(), 256); // 启动循环接收

    while (1)
    {
        /* code */
        usart_parse_loop();             // 非阻塞解析

        uint8_t tx[60];
        uint8_t len;
        if (odom_get_tx_frame(tx, &len))
        {
            HAL_UART_Transmit_DMA(&huart1, tx, len);
        }
    }
    
} 