//
// Created by CaoKangqi on 2026/6/20.
//

#ifndef H7_FRAMEWORK_CHASSIS_CTRL_H
#define H7_FRAMEWORK_CHASSIS_CTRL_H

#include <stdint.h>
#include "All_Motor.h"
#include "Chassis_Calc.h"
#include "DBUS.h"
#include "IMU_Task.h"

typedef struct {
    DJI_MOTOR_Typedef Steer[4];  // 4个舵轮的舵向电机
    DJI_MOTOR_Typedef Drive[4];  // 4个舵轮的驱动电机

    PID_t PID_Vx;
    PID_t PID_Vy;
    PID_t PID_Vw;

    Swerve_Feedback_t swerve_fb;   // 喂给解算器的标准输入结构体
    Swerve_Command_t  swerve_cmd;  // 解算器输出的标准输出结构体
} Chassis_Ctrl_Block_t;

uint8_t Chassis_Control_Init(void);
void Chassis_Control_Task(const Chassis_Motor_Group_t *p_motor,
                          const IMU_Data_t *p_imu_repo,
                          const DBUS_Typedef *p_dbus);

#endif //H7_FRAMEWORK_CHASSIS_CTRL_H
