//
// Created by CaoKangqi on 2026/6/20.
//
#include "Chassis_Ctrl.h"
#include "All_define.h"
#include "Power_Ctrl.h"
#include "System_State.h"

Power_Ctrl_t chassis_model;

static Chassis_Ctrl_Block_t chassis_ctrl;
// 加速度外环PID
Swerve_Cfg_t S_Cfg;
Swerve_State_t S_Now;

/**
 * @brief 底盘控制初始化
 * @param MOTOR 电机总结构体指针
 * @return uint8_t 初始化状态
 */
uint8_t Chassis_Control_Init(void)
{
    // 初始化舵轮物理配置及物理状态
    Swerve_Init(&S_Cfg, &S_Now);

    // 1. 底盘速度外环 PID 初始化 (输出为目标加速度)
    float PID_V_Param[3] = {8.0f, 0.0f, 0.0f};
    PID_Init(&chassis_ctrl.PID_Vx, 8.0f, 5.0f, PID_V_Param,
        0, 0, 0, 0, 0, Integral_Limit | ErrorHandle);
    PID_Init(&chassis_ctrl.PID_Vy, 8.0f, 5.0f, PID_V_Param,
        0, 0, 0, 0, 0, Integral_Limit | ErrorHandle);

    float PID_Vw_Param[3] = {2.0f, 0.0f, 0.0f};
    PID_Init(&chassis_ctrl.PID_Vw, 8.0f, 8.0f, PID_Vw_Param,
        0, 0, 0, 0, 0, Integral_Limit | ErrorHandle);

    // 2. 轮系单轴 PID 参数配置
    float PID_6020_Pos[3] = {800.0f, 0.0f, 0.0f};
    float PID_6020_Spd[3] = {85.0f,  0.0f, 0.0f};
    float PID_3508_Spd[3] = {5.0f,   0.1f,  0.0f};

    for (int i = 0; i < 4; i++)
    {
        // 6020 舵向位置环：输入弧度误差 -> 输出目标 RPM
        PID_Init(&chassis_ctrl.Steer[i].PID_P, 250.0f,  30.0f,  PID_6020_Pos,
            0, 0, 0, 0, 0, Integral_Limit | ErrorHandle);
        // 6020 舵向速度环：输入 RPM 误差 -> 输出电流
        PID_Init(&chassis_ctrl.Steer[i].PID_S, 16384.0f, 4000.0f, PID_6020_Spd,
            0, 0, 0, 0, 0, Integral_Limit | ErrorHandle);
        // 3508 驱动速度环：输入 RPM 误差 -> 输出电流
        PID_Init(&chassis_ctrl.Drive[i].PID_S, 16384.0f, 3000.0f, PID_3508_Spd,
            0, 0, 0, 0, 0, Integral_Limit | ErrorHandle);
    }

    Power_Ctrl_Init(&chassis_model);

    System_State_Report(ID_CHASSIS, STATUS_PREPARING);
    return 1;
}


void Chassis_Control_Task(const Chassis_Motor_Group_t *p_motor,
                          const IMU_Data_t *p_imu_repo,
                          const DBUS_Typedef *p_dbus)
{
    if (p_motor == NULL || p_imu_repo == NULL || p_dbus == NULL) {
        System_State_Report(ID_CHASSIS, STATUS_ERROR);
        return;
    }
    System_State_Report(ID_CHASSIS, STATUS_RUN);
    for (int i = 0; i < 4; i++) {
        chassis_ctrl.Steer[i].p_data = &p_motor->DJI_6020_Steer[i];
        chassis_ctrl.Drive[i].p_data = &p_motor->DJI_3508_Chassis[i];
        chassis_ctrl.swerve_fb.steer_angle_rad[i] = (float)chassis_ctrl.Steer[i].p_data->Angle_Infinite * ENCODER_TO_RAD;
        chassis_ctrl.swerve_fb.steer_rpm[i]       = (float)chassis_ctrl.Steer[i].p_data->Speed_now;
        chassis_ctrl.swerve_fb.wheel_rpm[i]       = (float)chassis_ctrl.Drive[i].p_data->Speed_now;
    }
    chassis_ctrl.swerve_fb.gyro_vw = -p_imu_repo->gyro[2];

    Swerve_Forward_Calc(&S_Now, &chassis_ctrl.swerve_fb, &S_Cfg);

    float vx_tar = p_dbus->Remote.CH1 * 0.003f + (float)p_dbus->KeyBoard.W * 1.0f - (float)p_dbus->KeyBoard.S * 1.0f;
    float vy_tar = p_dbus->Remote.CH0 * 0.003f + (float)p_dbus->KeyBoard.D * 1.0f - (float)p_dbus->KeyBoard.A * 1.0f;
    float vw_tar = p_dbus->Remote.CH2 * 0.02f  + (float)p_dbus->KeyBoard.E * 3.0f - (float)p_dbus->KeyBoard.Q * 3.0f + p_dbus->Mouse.X_Flt * 0.02f;

    PID_Calculate(&chassis_ctrl.PID_Vx, S_Now.vx, vx_tar);
    PID_Calculate(&chassis_ctrl.PID_Vy, S_Now.vy, vy_tar);
    PID_Calculate(&chassis_ctrl.PID_Vw, S_Now.vw, vw_tar);

    Swerve_Inverse_Calc(&chassis_ctrl.swerve_cmd, &S_Now,
                        chassis_ctrl.PID_Vx.Output, chassis_ctrl.PID_Vy.Output, chassis_ctrl.PID_Vw.Output,
                        vx_tar, vy_tar, vw_tar,
                        &chassis_ctrl.swerve_fb, &S_Cfg);

    for (int i = 0; i < 4; i++)
    {
        PID_Calculate(&chassis_ctrl.Steer[i].PID_P,
                      chassis_ctrl.swerve_fb.steer_angle_rad[i],
                      chassis_ctrl.swerve_cmd.target_steer_angle_rad[i]);

        PID_Calculate(&chassis_ctrl.Steer[i].PID_S,
                      chassis_ctrl.swerve_fb.steer_rpm[i],
                      chassis_ctrl.Steer[i].PID_P.Output);

        PID_Calculate(&chassis_ctrl.Drive[i].PID_S,
                      chassis_ctrl.swerve_fb.wheel_rpm[i],
                      chassis_ctrl.swerve_cmd.target_wheel_rpm[i]);

        chassis_ctrl.Drive[i].PID_S.Output += chassis_ctrl.swerve_cmd.ff_torque_raw[i];
    }

    for (int i = 0; i < 4; i++) {
        chassis_ctrl.Steer[i].PID_S.Output = MATH_Limit_float(16384,-16384, chassis_ctrl.Steer[i].PID_S.Output);
        chassis_ctrl.Drive[i].PID_S.Output = MATH_Limit_float(16384,-16384, chassis_ctrl.Drive[i].PID_S.Output);
    }

    DJI_Motor_Send(&hfdcan1, 0x200,
                   (int16_t)chassis_ctrl.Drive[0].PID_S.Output,
                   (int16_t)chassis_ctrl.Drive[1].PID_S.Output,
                   (int16_t)chassis_ctrl.Drive[2].PID_S.Output,
                   (int16_t)chassis_ctrl.Drive[3].PID_S.Output);

    DJI_Motor_Send(&hfdcan2, 0x1FE,
                   (int16_t)chassis_ctrl.Steer[0].PID_S.Output,
                   (int16_t)chassis_ctrl.Steer[1].PID_S.Output,
                   (int16_t)chassis_ctrl.Steer[2].PID_S.Output,
                   (int16_t)chassis_ctrl.Steer[3].PID_S.Output);
}