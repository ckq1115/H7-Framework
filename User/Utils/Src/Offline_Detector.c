//
// Created by qza on 2026/6/16.
//
#include "Offline_Detector.h"
#include "All_define.h"
#include "All_Motor.h"
#include "DBUS.h"
#include "Power_CAP.h"
#include "VT13.h"

static const Offline_Route_t Offline_Config_Table[] = {
    /* ----- 底盘组 ----- */
    /*{&chassis_motors.DJI_3508_Chassis[0].offline,  MOTOR_OFFLINE_TIME,  CHASSIS},
    {&chassis_motors.DJI_3508_Chassis[1].offline,  MOTOR_OFFLINE_TIME,  CHASSIS},
    {&chassis_motors.DJI_3508_Chassis[2].offline,  MOTOR_OFFLINE_TIME,  CHASSIS},
    {&chassis_motors.DJI_3508_Chassis[3].offline,  MOTOR_OFFLINE_TIME,  CHASSIS},*/
    {&chassis_motors.DJI_6020_Steer[0].offline,    MOTOR_OFFLINE_TIME,  CHASSIS},
    {&chassis_motors.DJI_6020_Steer[1].offline,    MOTOR_OFFLINE_TIME,  CHASSIS},
    {&chassis_motors.DJI_6020_Steer[2].offline,    MOTOR_OFFLINE_TIME,  CHASSIS},
    {&chassis_motors.DJI_6020_Steer[3].offline,    MOTOR_OFFLINE_TIME,  CHASSIS},

    /*/* ----- 云台组 ----- #1#
    {&gimbal_motors.DJI_3508_Yaw.offline,         MOTOR_OFFLINE_TIME,  GIMBAL},

    /* ----- 发射组 ----- #1#
    {&shoot_motors.DM4310_Feed.offline,          MOTOR_OFFLINE_TIME,  SHOOT},
    {&shoot_motors.DJI_3508_Pull.offline,        MOTOR_OFFLINE_TIME,  SHOOT},*/

    /* ----- 其他系统 ----- */
    {&DBUS.offline,                                DBUS_OFFLINE_TIME,   GROUP_NONE},
    //{&VT13.offline,                                DBUS_OFFLINE_TIME,   GROUP_NONE},
    {&Referee.offline,                       REFEREE_OFFLINE_TIME, GROUP_NONE},
    //{&cap.get.offline,                             CAP_OFFLINE_TIME,    GROUP_NONE},
    // 注意：如果有裁判系统，也可以在这里统一加上它的 offline 地址
};

void Offline_Monitor(void)
{
    uint32_t now = HAL_GetTick();
    size_t table_size = sizeof(Offline_Config_Table) / sizeof(Offline_Route_t);

    for (size_t i = 0; i < table_size; i++)
    {
        Offline_Check_t *dev = Offline_Config_Table[i].node;

        // 如果指针为空，或者超时配置为0，则跳过不查
        if (dev == NULL || Offline_Config_Table[i].timeout_ms == 0) continue;

        if ((now - dev->last_feed_tick) > Offline_Config_Table[i].timeout_ms) {
            dev->is_online = false;
        } else {
            dev->is_online = true;
        }
    }
}

bool Is_Group_Online(Device_Group_e group)
{
    size_t table_size = sizeof(Offline_Config_Table) / sizeof(Offline_Route_t);

    for (size_t i = 0; i < table_size; i++)
    {
        // 如果查全组，或者目标组匹配
        if (group == GROUP_ALL || Offline_Config_Table[i].group == group)
        {
            Offline_Check_t *dev = Offline_Config_Table[i].node;
            if (dev != NULL && dev->is_online == false) {
                return false; // 只要有一个掉线，直接返回 false
            }
        }
    }
    return true;
}