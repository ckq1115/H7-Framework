//
// Created by CaoKangqi on 2026/6/14.
//
#include "All_Task.h"


static uint32_t INS_DWT_Count = 0; // DWT计数基准
static float imu_period_s = 0.0f;

void IMU_Task(void *argument)
{
    (void)argument;
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xTimeIncrement = pdMS_TO_TICKS(1);
    INS_DWT_Count = DWT->CYCCNT;
    for(;;)
    {
        vTaskDelayUntil(&xLastWakeTime, xTimeIncrement);
        imu_period_s = DWT_GetDeltaT(&INS_DWT_Count);
        BMI088_read(IMU_Data.gyro, IMU_Data.accel, &IMU_Data.temp);
        IMU_Update_Task(imu_period_s);
    }
}

