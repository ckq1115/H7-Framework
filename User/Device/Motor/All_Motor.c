//
// Created by CaoKangqi on 2026/6/19.
//
#include "All_Motor.h"

Chassis_Motor_Group_t chassis_motors;
Gimbal_Motor_Group_t  gimbal_motors;
Shoot_Motor_Group_t   shoot_motors;

/**
 * @brief 通用能量积分堵转检测
 * @param current_feedback [输入] 当前电流/转矩
 * @param speed_feedback   [输入] 当前速度
 * @param energy_limit     [参数] 堵转能量阈值（控制触发的灵敏度，通常取 100.0f ~ 1000.0f）
 * @param p_stuck_energy   [指针/状态] 指向外部维护的 float 能量累加器变量
 * @return uint8_t         0 -> 正常； 1 -> 确定堵转
 */
uint8_t Motor_Stuck_Check(float current_feedback, float speed_feedback,
                                 float energy_limit, float *p_stuck_energy)
{
    if (p_stuck_energy == NULL) return 0;
    // 1. 预设电机的物理硬阈值（因为是通用的，可以直接在这里定死一个普遍标准）
    // 比如：大电流定义为额定电流的 70% 以上，低速定义为接近 0
    const float C_LIMIT = 10000.0f; // 针对大疆3508/6020等电机的通用电流阈值
    const float S_LIMIT = 15.0f;    // 通用低速阈值 (15 rpm)
    // 2. 判断当前是否处于“疑似堵转”状态
    if (fabsf(current_feedback) > C_LIMIT && fabsf(speed_feedback) < S_LIMIT)
    {
        // 疑似堵转：能量快速累加（桶里加水）
        *p_stuck_energy += 1.0f;

        // 限制能量上限，防止溢出后无法恢复
        if (*p_stuck_energy > energy_limit) {
            *p_stuck_energy = energy_limit;
            return 1; // 能量爆表，确定堵转！
        }
    }
    else
    {
        // 运行正常：能量缓慢衰减（桶自动漏水）
        // 乘以 0.95f 是一种非常平滑的指数衰减，能自动过滤偶发性的卡顿
        *p_stuck_energy *= 0.95f;
        if (*p_stuck_energy < 0.1f) {
            *p_stuck_energy = 0.0f;
        }
    }
    return 0; // 正常
}