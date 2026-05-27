#include "tim.h"
#include "odom.h"          // 别忘了声明

void TIM2_IRQHandler(void)
{
    if (__HAL_TIM_GET_FLAG(&htim2, TIM_FLAG_UPDATE))
    {
        __HAL_TIM_CLEAR_FLAG(&htim2, TIM_FLAG_UPDATE);
        can_send_sync();          // 1. 先广播 SYNC
        odom_update_10ms();       // 2. 再读 TPDO 实际值
    }
}

// CubeMX把TIM2配成100Hz（周期10ms）并打开全局中断即可
//上电后在main里启动定时器
// HAL_TIM_Base_Start_IT(&htim2);   // 10 ms 基准

// 之后MCU就会每10ms自动进一次 TIM2_IRQHandler，从而周期性完成
// 把最新 target.vx/target.wz 反算成左右轮目标 → 发 CAN
