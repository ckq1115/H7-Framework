//
// Created by CaoKangqi on 2026/6/17.
#include "Comm_Router.h"
#include "All_Motor.h"
#include "BSP_UART.h"
#include "Buzzer.h"
#include "DBUS.h"
#include "IMU_Task.h"
#include "Power_CAP.h"
#include "System_State.h"
#include "Vofa.h"
#include "VT13.h"
#include "WS2812.h"

static const CAN_Rx_Route_t CAN_Rx_Config_Table[] = {
    /* ----- FDCAN1 ----- */
    //           总线        ID             目标结构体指针                          解析函数
    {FDCAN1, 0x201, &chassis_motors.DJI_3508_Chassis[0], DJI_Motor_Resolve},
    {FDCAN1, 0x202, &chassis_motors.DJI_3508_Chassis[1], DJI_Motor_Resolve},
    {FDCAN1, 0x203, &chassis_motors.DJI_3508_Chassis[2], DJI_Motor_Resolve},
    {FDCAN1, 0x204, &chassis_motors.DJI_3508_Chassis[3], DJI_Motor_Resolve},

    /* ----- FDCAN2 ----- */
    {FDCAN2, 0x205, &chassis_motors.DJI_6020_Steer[0],   DJI_Motor_Resolve},
    {FDCAN2, 0x206, &chassis_motors.DJI_6020_Steer[1],   DJI_Motor_Resolve},
    {FDCAN2, 0x207, &chassis_motors.DJI_6020_Steer[2],   DJI_Motor_Resolve},
    {FDCAN2, 0x208, &chassis_motors.DJI_6020_Steer[3],   DJI_Motor_Resolve},

    /* ----- FDCAN3 ----- */
    {FDCAN3, 0x301, &shoot_motors.DM4310_Feed,         DM_1to4_Resolve},
    {FDCAN3, 0x202, &shoot_motors.DJI_3508_Pull,       DJI_Motor_Resolve},
    {FDCAN3, 0x203, &gimbal_motors.DJI_3508_Yaw,        DJI_Motor_Resolve},
    {FDCAN3, 0x288, &cap.get,                            Power_Cap_Rx},
};

static const UART_Rx_Route_t UART_Rx_Config_Table[] = {
    // 总线                  预期长度      主Buffer           备用Buffer(双缓冲)                  DMA重载长度                       目标结构体指针       解析函数
    {&huart5,  18, DBUS_RX_DATA,      NULL,              18,                     &DBUS, DBUS_Resolved},
    {&huart7,  21, VT13_RX_DATA,      NULL,              21,                     &VT13, VT13_Resolved},
    {&huart1,  0, Referee_Rx_Buf[0], Referee_Rx_Buf[1], REFEREE_RXFRAME_LENGTH, NULL,  Referee_System_Frame_Update},
};

void MY_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    if (htim->Instance == TIM4) {
        WS2812_Ticks_1ms();
        DWT_SysTimeUpdate();
        Offline_Monitor();
        System_State_Update(&DBUS.offline);
        VOFA_JustFloat(4,IMU_Data.pitch,IMU_Data.roll,IMU_Data.yaw,0);
    }
}

void CAN_Router_Init(void)
{
    size_t table_size = sizeof(CAN_Rx_Config_Table) / sizeof(CAN_Rx_Route_t);
    for (size_t i = 0; i < table_size; i++)
    {
        FDCAN_HandleTypeDef temp_hfdcan = {0};
        temp_hfdcan.Instance = CAN_Rx_Config_Table[i].instance;
        BSP_CAN_Register_Slot(
            &temp_hfdcan,
            CAN_Rx_Config_Table[i].id,
            CAN_Rx_Config_Table[i].device_ptr,
            CAN_Rx_Config_Table[i].resolve
        );
    }
}

void UART_Router_Init(void)
{
    size_t table_size = sizeof(UART_Rx_Config_Table) / sizeof(UART_Rx_Route_t);
    for (size_t i = 0; i < table_size; i++)
    {
        BSP_UART_Register_Slot(
            UART_Rx_Config_Table[i].huart,
            UART_Rx_Config_Table[i].expected_size,
            UART_Rx_Config_Table[i].rx_buf0,
            UART_Rx_Config_Table[i].rx_buf1,
            UART_Rx_Config_Table[i].dma_rx_size,
            UART_Rx_Config_Table[i].device_ptr,
            UART_Rx_Config_Table[i].resolve
        );
    }
}