//
// Created by CaoKangqi on 2026/6/14.
//
#include "All_Task.h"
#include "All_Motor.h"
#include "Buzzer.h"
#include "Chassis_Ctrl.h"
#include "DBUS.h"
#include "Message_Center.h"
#include "Robot_Cmd.h"
#include "System_State.h"
#include "WS2812.h"

//指令中心任务 200Hz
void Command_Task(void *argument)
{
    (void)argument;
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xTimeIncrement = pdMS_TO_TICKS(5);//绝对延时5ms

    Robot_Cmd_Init();
    for(;;)
    {
        vTaskDelayUntil(&xLastWakeTime, xTimeIncrement);

        Robot_Cmd_Update();
    }
}

//IMU姿态解算任务 1000Hz
void IMU_Task(void *argument)
{
    (void)argument;
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xTimeIncrement = pdMS_TO_TICKS(1);//绝对延时1ms

    static uint32_t INS_DWT_Count = 0; // DWT计数基准
    static float imu_period_s = 0.0f;
    INS_DWT_Count = DWT->CYCCNT;
    for(;;)
    {
        vTaskDelayUntil(&xLastWakeTime, xTimeIncrement);

        imu_period_s = DWT_GetDeltaT(&INS_DWT_Count);
        IMU_Update_Task(&IMU_Data,imu_period_s);
    }
}

//运动控制任务 1000Hz
static IMU_Data_t imu ={0};
static Chassis_Motor_Group_t chassis_m = {0};
void Motor_Task(void *argument)
{
    (void)argument;
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xTimeIncrement = pdMS_TO_TICKS(1);//绝对延时1ms

    Subscriber_t *imu_sub = NULL;
    Subscriber_t *motor_sub = NULL;

    imu_sub = SubRegister("imu_data", sizeof(IMU_Data_t));
    motor_sub = SubRegister("chassis_motors", sizeof(Chassis_Motor_Group_t));
    Chassis_Control_Init();
    for(;;)
    {
        vTaskDelayUntil(&xLastWakeTime, xTimeIncrement);

        if (imu_sub) SubGetMessage(imu_sub, &imu);
        if (motor_sub) SubGetMessage(motor_sub, &chassis_m);
        Chassis_Control_Task(&chassis_m,&imu);
    }
}

//定时器中断
void MY_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    //定时器4 1000Hz
    if (htim->Instance == TIM4) {
        WS2812_Ticks();
        DWT_SysTimeUpdate();
        Offline_Monitor();
        System_State_Update();
        System_State_Ticks();
    }
}