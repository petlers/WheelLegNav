/* can.c */
#include "can.h"
extern CAN_HandleTypeDef hcan1;

void MX_CAN_Init(void)
{
    hcan1.Instance = CAN1;
    hcan1.Init.Prescaler = 9;
    hcan1.Init.Mode = CAN_MODE_NORMAL;
    hcan1.Init.SyncJumpWidth = CAN_SJW_1TQ;
    hcan1.Init.TimeSeg1 = CAN_BS1_5TQ;
    hcan1.Init.TimeSeg2 = CAN_BS2_2TQ;
    hcan1.Init.TimeTriggeredMode = DISABLE;
    hcan1.Init.AutoBusOff = ENABLE;
    hcan1.Init.AutoWakeUp = ENABLE;
    HAL_CAN_Init(&hcan1);

    CAN_FilterTypeDef f = {0};
    f.FilterIdHigh         = 0;
    f.FilterIdLow          = 0;
    f.FilterMaskIdHigh     = 0;
    f.FilterMaskIdLow      = 0;
    f.FilterFIFOAssignment = CAN_RX_FIFO0;
    f.FilterBank           = 0;
    f.FilterMode           = CAN_FILTERMODE_IDMASK;
    f.FilterScale          = CAN_FILTERSCALE_32BIT;
    f.FilterActivation     = ENABLE;
    HAL_CAN_ConfigFilter(&hcan1, &f);
    HAL_CAN_Start(&hcan1);
    HAL_CAN_ActivateNotification(&hcan1, CAN_IT_RX_FIFO0_MSG_PENDING);
}

/* 发 RPDO1 = 0x200+NodeID  DLC=2  目标速度 0.1 rpm / LSB */
void can_send_target(uint8_t node_id, int16_t rpm_0p1)
{
    CAN_TxHeaderTypeDef h;
    uint32_t mb;
    
    // 0x201是左电机， 0x202是右电机
    h.StdId = 0x200 + node_id;
    
    h.IDE   = CAN_ID_STD;
    h.RTR   = CAN_RTR_DATA;
    h.DLC   = 2;
    uint8_t d[2] = {rpm_0p1 & 0xFF, (rpm_0p1 >> 8) & 0xFF};
    HAL_CAN_AddTxMessage(&hcan1, &h, d, &mb);
}

/* 10 ms SYNC 发送 */
// SYNC不需要“配置”，它天生就是一条固定格式的裸帧 ID = 0x080，DLC = 0，数据段为空
// 只要你的代码周期性地把这条帧丢到总线上，所有支持 CiA-402 的驱动器就会自动响应——没有任何寄存器要配。
void can_send_sync(void)
{
    CAN_TxHeaderTypeDef h;
    uint32_t mb;
    h.StdId = 0x080;
    h.IDE   = CAN_ID_STD;   // IDE（Identifier Extension）；取值CAN_ID_STD表示用11位标准ID（不是29位拓展ID）
    h.RTR   = CAN_RTR_DATA; // RTR（Remote Transmission Request）；取值CAN_RTR_DATA表示这是数据帧（不是远程帧）
    h.DLC   = 0;            // DLC（Data Length Code）；取值为0表示数据区长度为0字节
    HAL_CAN_AddTxMessage(&hcan1, &h, 0, &mb); // &hcan1 使用can1外设。   
}

// 没有SYNC，电机就会左脚先迈，右脚后迈。 因为CAN报文天生有仲裁延迟（优先级高的先发），如果两个电机收到RPDO后，右轮仲裁胜，会抖一下
// 加了SYNC，所有电机同时抬脚，同时落地。 收到SYNC（0x080）那一刻，所有电机ID同时锁存RPDO里的目标值，并把实际值通过TPDO发回来。完整流程如下：
// 完整流程：
// t=0 ms，MCU 依次发给 0x201、0x202 目标速度 → 只写入驱动器缓存，不执行
// t=1 ms，MCU 发 SYNC（0x080），所有驱动器同时把缓存值写进 0x60FF，电机一起换速；同时把实际速度通过 TPDO 发出
// 结果，四台电机零时间差，机器人平稳，里程计积分可信

