#ifndef G4_FRAMEWORK_POWER_CTRL_H
#define G4_FRAMEWORK_POWER_CTRL_H

#include <stdint.h>

#define POWER_RPM_TO_RAD       (2.0f * 3.14159265f / 60.0f)

// 电机功率物理模型参数
typedef struct {
    float k1, k2, k3, k4;
    float current_convert;
} Power_Motor_Model_t;

// 输入的单电机状态
typedef struct {
    float speed_rpm;       // 当前实时转速 (RPM)
    float original_cmd;    // 原始输入的 PID 电流控制项
    float limited_cmd;     // 缩放限制后的电流控制项
} Motor_Power_State_t;

// 功率控制器
typedef struct {
    float Kp;
    float target_buffer;
    Power_Motor_Model_t m3508;
    Power_Motor_Model_t m6020;

    float total_pred_power;   // 解算后预测的总功率
} Power_Ctrl_t;

void Power_Ctrl_Init(Power_Ctrl_t *ctrl);

/**
 * @brief 核心控制算法（纯数学解算）
 * @param ctrl          算法实例
 * @param allowed_limit 当前解算允许的最大绝对功率 (瓦特 W)
 * @param cur_buffer    当前的裁判系统缓冲能量 (焦耳 J)
 * @param m3508_group   底盘3508电机的速度与输入电流集合 (4个)
 * @param m6020_group   底盘6020舵电机的速度与输入电流集合 (4个)
 */
void Power_Ctrl_Calculate(Power_Ctrl_t *ctrl,
                           float allowed_limit,
                           float cur_buffer,
                           Motor_Power_State_t m3508_group[4],
                           Motor_Power_State_t m6020_group[4]);

#endif // G4_FRAMEWORK_POWER_CTRL_H