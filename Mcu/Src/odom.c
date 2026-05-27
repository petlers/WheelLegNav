/* odom.c */
#include "odom.h"
#include "can.h"
#include <string.h>
#include <math.h>

#define BASE 0.30f
#define RADIUS 0.065f
#define CPR  8192.0f   // 编码器一圈脉冲
#define GEAR 36.0f     // 减速比

static Speed_t target;
static float  x = 0, y = 0, yaw = 0;
static float  vl_act = 0, vr_act = 0; // m/s
static uint8_t tx_frame[60];

void odom_init(void)
{
    memset(tx_frame, 0, sizeof(tx_frame));
}

void odom_set_target(Speed_t *sp) 
{ 
    target = *sp; 
}

/* 每 10 ms 调用一次 */
void odom_update_10ms(void)
{
    /* 1 运动学反算 → 目标 rpm */
    float vl = target.vx - target.wz * BASE * 0.5f;
    float vr = target.vx + target.wz * BASE * 0.5f;
    int16_t left  = (int16_t)(vl * 60.0f / (2.0f * M_PI * RADIUS) * 10.0f); // 0.1 rpm
    int16_t right = (int16_t)(vr * 60.0f / (2.0f * M_PI * RADIUS) * 10.0f);
    can_send_target(1, left);
    can_send_target(2, right);

    /* 2 从驱动器取实际速度（这里简化：用目标代替，真实可读 0x606C）*/
    vl_act = vl;
    vr_act = vr;
    float v  = (vl_act + vr_act) * 0.5f;
    float w  = (vr_act - vl_act) / BASE;
    /* 积分 */
    yaw += w * 0.01f;
    x   += v * cosf(yaw) * 0.01f;
    y   += v * sinf(yaw) * 0.01f;

    /* 3 构造回传帧  0xAA 55 58 0x02 ... XOR */
    tx_frame[0] = 0xAA;
    tx_frame[1] = 0x55;
    tx_frame[2] = 58;                 // 负载 56 B
    tx_frame[3] = 0x02;               // 类型
    memcpy(&tx_frame[4], &x, 4);
    memcpy(&tx_frame[8], &y, 4);
    memcpy(&tx_frame[12], &yaw, 4);
    memcpy(&tx_frame[16], &v, 4);
    memcpy(&tx_frame[20], &w, 4);
    /* IMU 占位  zeros */
    uint16_t crc = 0;
    for (int i = 2; i < 59; i++) crc += tx_frame[i];
    tx_frame[59] = crc & 0xFF;
}

bool odom_get_tx_frame(uint8_t *buf, uint8_t *len)
{
    static uint32_t last = 0;
    if (HAL_GetTick() - last < 50) return false; // 20 Hz 回传
    last = HAL_GetTick();
    memcpy(buf, tx_frame, 60);
    *len = 60;
    return true;
}