//
// Created by CaoKangqi on 2026/6/19.
//

#ifndef H7_FRAMEWORK_SYSTEM_STATE_H
#define H7_FRAMEWORK_SYSTEM_STATE_H

#include <stdint.h>
#include <stdbool.h>
#include "Offline_Detector.h" // 完美引入离线检测层

typedef enum {
    STATUS_INIT = 0,     // 初始化阶段（上电复位、内存分配、变量清零中）
    STATUS_PREPARING,    // 硬件准备/标定中（例如：IMU正在加热校准、电机正在找机械零位）
    STATUS_RUN,          // 正常运行中
    STATUS_LOST,         // 模块掉线/通信超时（警告状态，通常可尝试自动恢复）
    STATUS_ERROR         // 模块错误（设备彻底离线，无法恢复）
} App_Status_e;

typedef struct {
    App_Status_e IMU;
    App_Status_e Chassis;
    App_Status_e Shoot;
    App_Status_e Gimbal;
    App_Status_e Vision;
} App_Health_Table_t;

// 全局控制模式
typedef enum {
    GLOBAL_SAFE_LOCK = 0,   // 全车安全锁定
    GLOBAL_STANDBY,         // 全车待命
    GLOBAL_NORMAL_MATCH,    // 正常遥控比赛/战斗模式
} Global_Mode_e;

// 32 位故障位域映射
typedef union {
    struct {
        uint32_t imu_fault      : 1;  // Bit 0: IMU 任务层报错或死锁
        uint32_t remote_lost    : 1;  // Bit 1: 遥控器掉线
        uint32_t chassis_offline : 1; // Bit 2: 底盘硬件组有电机掉线 (新增)
        uint32_t gimbal_offline  : 1; // Bit 3: 云台硬件组有电机掉线 (新增)
        uint32_t shoot_offline   : 1; // Bit 4: 发射硬件组有电机掉线 (新增)
        uint32_t vision_lost    : 1;  // Bit 5: 视觉上位机通信丢失
        uint32_t referee_lost   : 1;  // Bit 6: 裁判系统掉线
        uint32_t reserved       : 25;
    } bit;
    uint32_t all_code;
} System_Error_Code_u;

//系统状态
typedef struct {
    App_Health_Table_t  task_health;   // 各任务组件层健康表
    Global_Mode_e       global_mode;   // 全局模式
    System_Error_Code_u error;         // 统一对外故障码
    float               power_limit;   // 功率限制
} System_State_t;

extern System_State_t sys_state;

//模块 ID 枚举
typedef enum {
    ID_IMU = 0,
    ID_CHASSIS,
    ID_GIMBAL,
    ID_SHOOT,
    ID_VISION
} Module_ID_e;

void System_State_Init(void);
void System_State_Report(Module_ID_e module_id, App_Status_e status);
void System_State_Update(Offline_Check_t *remote_offline);

#endif //H7_FRAMEWORK_SYSTEM_STATE_H