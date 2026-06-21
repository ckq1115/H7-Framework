//
// Created by CaoKangqi on 2026/6/14.
//
#include "All_Task.h"
#include "All_Motor.h"
#include "Buzzer.h"
#include "Chassis_Ctrl.h"
#include "DBUS.h"
#include "Message_Center.h"

void IMU_Task(void *argument)
{
    (void)argument;
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xTimeIncrement = pdMS_TO_TICKS(1);
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

static IMU_Data_t imu ={0};
static DBUS_Typedef chassis_dbus = {0};
static Chassis_Motor_Group_t chassis_m = {0};
void Motor_Task(void *argument)
{
    (void)argument;

    Subscriber_t *imu_sub = NULL;
    Subscriber_t *dbus_sub = NULL;
    Subscriber_t *motor_sub = NULL;

    imu_sub = SubRegister("imu_data", sizeof(IMU_Data_t));
    dbus_sub = SubRegister("dbus_data", sizeof(DBUS_Typedef));
    motor_sub = SubRegister("chassis_motors", sizeof(Chassis_Motor_Group_t));
    Chassis_Control_Init();
    Buzzer_Trigger_Status(BUZZER_STATUS_INIT_SUCCESS);
    for(;;)
    {
        if (imu_sub != NULL){
            SubGetMessage(imu_sub, &imu);
        }
        if (dbus_sub != NULL) {
            SubGetMessage(dbus_sub, &chassis_dbus);
        }
        if (motor_sub != NULL) {
            SubGetMessage(motor_sub, &chassis_m);
        }
        Chassis_Control_Task(&chassis_m,&imu,&chassis_dbus);
        Buzzer_Ticks_1ms();
        osDelay(1);
    }
}