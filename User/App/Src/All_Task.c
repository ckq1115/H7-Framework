//
// Created by CaoKangqi on 2026/6/14.
//
#include "All_Task.h"


static uint32_t INS_DWT_Count = 0; // DWT计数基准
static float imu_period_s = 0.0f;
void IMU_Task(void *argument)
{
    (void)argument;

    INS_DWT_Count = DWT->CYCCNT;
    for(;;)
    {
        imu_period_s = DWT_GetDeltaT(&INS_DWT_Count);
        BMI088_read(IMU_Data.gyro, IMU_Data.accel, &IMU_Data.temp);
        //IMU_Update_Task(imu_period_s);
        osDelay(1);
    }
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size){
    //uint8_t *pData = huart->pRxBuffPtr;
    if (huart->Instance == UART5){
        if (Size == 18){
            DBUS_Resolved(DBUS_RX_DATA, &DBUS);
            __HAL_DMA_DISABLE_IT(huart5.hdmarx, DMA_IT_HT);
        }
    }
}
void HAL_UART_ErrorCallback(UART_HandleTypeDef * huart){
    if (huart->Instance == UART5){
        UART_ReceiveToIdle_DMA(&huart5,DBUS_RX_DATA,18);
    }
}