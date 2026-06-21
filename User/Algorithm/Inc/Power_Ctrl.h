#ifndef G4_FRAMEWORK_POWER_CTRL_H
#define G4_FRAMEWORK_POWER_CTRL_H

#include <stdint.h>

#include "All_Motor.h"
#include "Power_CAP.h"
#include "Referee.h"


#define POWER_RPM_TO_RAD       (2.0f * 3.14159265f / 60.0f)
#define CAP_MIN_CAPACITY       23.0f   // 电容电量下限
#define CAP_THRESHOLD_CAPACITY 27.0f
#define CAP_SUPER_POWER_MAX    150.0f  // 电容最大释放功率
#define DEFAULT_POWER_LIMIT    50.0f   // 默认底盘功率限制
#define MAX_BUFFER_ENERGY      60.0f   // 最大缓冲能量

// 电机物理模型参数
typedef struct {
    float k1, k2, k3, k4;
    float current_convert;
} Power_Motor_Model_t;

// 功率计测量数据
typedef struct {
    float shunt_volt;
    float bus_volt;
    float current;
    float power;           // 当前实际总功率
    float buffer_energy;   // 本地解算的缓冲能量
} Power_Meter_t;

// 功率控制器主实例结构体
typedef struct {
    /* --- 参数区 --- */
    float Kp;                  // 缓冲能量PI补偿的比例系数
    float target_buffer;       // 期望维持的缓冲能量目标
    Power_Motor_Model_t m3508;
    Power_Motor_Model_t m6020;

    /* --- 状态区 --- */
    uint8_t cap_mode;          // 电容模式 (0:关闭, 1:开启)
    uint8_t cap_is_open;       // 当前电容是否正在输出的标志位
    float   basic_limit;       // 裁判系统基础限制 + 缓冲补偿
    float   actual_limit;      // 最终用于模型解算的功率上限
    float   total_pred_power;  // 解算后预测的总功率

    /* --- 模块句柄 --- */
    Power_Meter_t meter;       // 功率计数据
} Power_Ctrl_Instance_t;

// --- 外部接口 ---
void Power_Ctrl_Init(Power_Ctrl_Instance_t *ctrl);
void CAN_Power_Rx(Power_Ctrl_Instance_t *ctrl, uint8_t *rx_data);
void Buffer_Calc(Power_Ctrl_Instance_t *ctrl, float dt, float ref_power_limit);
uint8_t Power_Ctrl_Execute(Power_Ctrl_Instance_t *ctrl, User_Data_T *referee, Cap_t *cap, MOTOR_Typdef *motors);

#endif // G4_FRAMEWORK_POWER_CTRL_H