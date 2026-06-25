//
// Created by CaoKangqi on 2026/6/25.
//
#include "Offline_Detector.h"
#include "BSP_UART.h"
#include "BSP_FDCAN.h"
#include "All_define.h"
#include "DBUS.h"
#include "VT13.h"
#include "Referee.h"
#include "Robot_Config.h"
#include "Comm_DualBoard.h"
#include "Power_CAP.h"

Chassis_Motor_Group_t chassis_motors;
Gimbal_Motor_Group_t  gimbal_motors;
Shoot_Motor_Group_t   shoot_motors;



EXPORT_UART_RX_NODE(&huart5, 18, DBUS_RX_DATA, NULL, 18, &DBUS, DBUS_Resolved);
EXPORT_OFFLINE_NODE(&DBUS.offline, DBUS_OFFLINE_TIME, GROUP_NONE);

// 【VT13 图传】绑定到 UART7
EXPORT_UART_RX_NODE(&huart7, 21, VT13_RX_DATA, NULL, 21, &VT13, VT13_Resolved);
EXPORT_OFFLINE_NODE(&VT13.offline, DBUS_OFFLINE_TIME, GROUP_NONE);

// 【裁判系统】绑定到 UART1，双缓冲模式
EXPORT_UART_RX_NODE(&huart1, 0, Referee_Rx_Buf[0], Referee_Rx_Buf[1], REFEREE_RXFRAME_LENGTH, NULL, Referee_System_Frame_Update);
EXPORT_OFFLINE_NODE(&Referee.offline, REFEREE_OFFLINE_TIME, GROUP_NONE);

/* -------------------- CAN 设备拓扑映射 -------------------- */

EXPORT_CAN_RX_NODE(FDCAN1, 0x201, &chassis_motors.DJI_3508_Chassis[0], DJI_Motor_Resolve);
EXPORT_OFFLINE_NODE(&chassis_motors.DJI_3508_Chassis[0].offline, MOTOR_OFFLINE_TIME, CHASSIS);

EXPORT_CAN_RX_NODE(FDCAN1, 0x202, &chassis_motors.DJI_3508_Chassis[1], DJI_Motor_Resolve);
EXPORT_OFFLINE_NODE(&chassis_motors.DJI_3508_Chassis[1].offline, MOTOR_OFFLINE_TIME, CHASSIS);

EXPORT_CAN_RX_NODE(FDCAN1, 0x203, &chassis_motors.DJI_3508_Chassis[2], DJI_Motor_Resolve);
EXPORT_OFFLINE_NODE(&chassis_motors.DJI_3508_Chassis[2].offline, MOTOR_OFFLINE_TIME, CHASSIS);

EXPORT_CAN_RX_NODE(FDCAN1, 0x204, &chassis_motors.DJI_3508_Chassis[3], DJI_Motor_Resolve);
EXPORT_OFFLINE_NODE(&chassis_motors.DJI_3508_Chassis[3].offline, MOTOR_OFFLINE_TIME, CHASSIS);

// 2. 底盘转向电机 (DJI 6020) - 挂载于 FDCAN2
EXPORT_CAN_RX_NODE(FDCAN2, 0x205, &chassis_motors.DJI_6020_Steer[0], DJI_Motor_Resolve);
EXPORT_OFFLINE_NODE(&chassis_motors.DJI_6020_Steer[0].offline, MOTOR_OFFLINE_TIME, CHASSIS);

EXPORT_CAN_RX_NODE(FDCAN2, 0x206, &chassis_motors.DJI_6020_Steer[1], DJI_Motor_Resolve);
EXPORT_OFFLINE_NODE(&chassis_motors.DJI_6020_Steer[1].offline, MOTOR_OFFLINE_TIME, CHASSIS);

EXPORT_CAN_RX_NODE(FDCAN2, 0x207, &chassis_motors.DJI_6020_Steer[2], DJI_Motor_Resolve);
EXPORT_OFFLINE_NODE(&chassis_motors.DJI_6020_Steer[2].offline, MOTOR_OFFLINE_TIME, CHASSIS);

EXPORT_CAN_RX_NODE(FDCAN2, 0x208, &chassis_motors.DJI_6020_Steer[3], DJI_Motor_Resolve);
EXPORT_OFFLINE_NODE(&chassis_motors.DJI_6020_Steer[3].offline, MOTOR_OFFLINE_TIME, CHASSIS);


EXPORT_CAN_RX_NODE(FDCAN3, 0x301, &shoot_motors.DM4310_Feed, DM_1to4_Resolve);
EXPORT_OFFLINE_NODE(&shoot_motors.DM4310_Feed.offline, MOTOR_OFFLINE_TIME, SHOOT);

// 2. 发射机构：摩擦轮/拉弹电机 (DJI 3508) - 挂载于 FDCAN3
EXPORT_CAN_RX_NODE(FDCAN3, 0x202, &shoot_motors.DJI_3508_Pull, DJI_Motor_Resolve);
EXPORT_OFFLINE_NODE(&shoot_motors.DJI_3508_Pull.offline, MOTOR_OFFLINE_TIME, SHOOT);

// 3. 云台机构：Yaw轴电机 (DJI 3508) - 挂载于 FDCAN3
EXPORT_CAN_RX_NODE(FDCAN3, 0x203, &gimbal_motors.DJI_3508_Yaw, DJI_Motor_Resolve);
EXPORT_OFFLINE_NODE(&gimbal_motors.DJI_3508_Yaw.offline, MOTOR_OFFLINE_TIME, GIMBAL);


EXPORT_CAN_RX_NODE(FDCAN2, 0x500, &Rx_Data, DualBoard_CAN_Rx);

EXPORT_CAN_RX_NODE(FDCAN3, 0x288, &cap, Power_Cap_Rx);
EXPORT_OFFLINE_NODE(&cap.get.offline, CAP_OFFLINE_TIME, GROUP_NONE);