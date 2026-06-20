//
// Created by CaoKangqi on 2026/6/19.
//

#include "System_State.h"
#include "DBUS.h"
#include "Referee.h"

System_State_t sys_state;

void System_State_Init(void) {
    // 默认全车任务状态初始化为 STATUS_INIT
    sys_state.task_health.IMU     = STATUS_INIT;
    sys_state.task_health.Chassis = STATUS_INIT;
    sys_state.task_health.Gimbal  = STATUS_INIT;
    sys_state.task_health.Shoot   = STATUS_INIT;
    sys_state.task_health.Vision  = STATUS_INIT;

    // 默认全局运行模式安全锁死
    sys_state.global_mode = GLOBAL_SAFE_LOCK;

    // 错误及限制初始化
    sys_state.error.all_code = 0;
    sys_state.power_limit = 45.0f;
}

void System_State_Report(Module_ID_e module_id, App_Status_e status) {
    switch (module_id) {
        case ID_IMU:     sys_state.task_health.IMU     = status; break;
        case ID_CHASSIS: sys_state.task_health.Chassis = status; break;
        case ID_GIMBAL:  sys_state.task_health.Gimbal  = status; break;
        case ID_SHOOT:   sys_state.task_health.Shoot   = status; break;
        case ID_VISION:  sys_state.task_health.Vision  = status; break;
    }
}

/**
 * @brief 系统状态集中裁决更新
 * @note  融合了底层离线组状态监测 Is_Group_Online
 */
void System_State_Update(Offline_Check_t *remote_offline) {

    // 遥控器在线检测
    sys_state.error.bit.remote_lost   = (remote_offline != NULL) ? !remote_offline->is_online : true;
    // 判断各执行机构底层是否有设备离线
    sys_state.error.bit.chassis_offline = !Is_Group_Online(CHASSIS);
    sys_state.error.bit.gimbal_offline  = !Is_Group_Online(GIMBAL);
    sys_state.error.bit.shoot_offline   = !Is_Group_Online(SHOOT);

    // 统计应用层任务（如 IMU、视觉）的状态报错
    sys_state.error.bit.imu_fault     = (sys_state.task_health.IMU == STATUS_ERROR);
    sys_state.error.bit.vision_lost    = (sys_state.task_health.Vision == STATUS_LOST);

    if (sys_state.error.bit.remote_lost ||
        sys_state.error.bit.imu_fault   ||
        sys_state.error.bit.chassis_offline ||
        sys_state.error.bit.gimbal_offline)
    {
        sys_state.global_mode = GLOBAL_SAFE_LOCK;
        return;
    }
}