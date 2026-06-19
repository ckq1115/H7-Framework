//
// Created by CaoKangqi on 2026/6/17.
//
#include "Interrupt.h"

static const CAN_Rx_Route_t CAN_Rx_Config_Table[] = {
    /* ----- FDCAN1 ----- */
    //           总线        ID             目标结构体指针                          解析函数                   离线超时时间       所属分组
    {FDCAN1, 0x201, &All_Motor.DJI_3508_Chassis[0].DATA, DJI_Motor_Resolve,  MOTOR_OFFLINE_TIME,   CHASSIS},
    {FDCAN1, 0x202, &All_Motor.DJI_3508_Chassis[1].DATA, DJI_Motor_Resolve,  MOTOR_OFFLINE_TIME,   CHASSIS},
    {FDCAN1, 0x203, &All_Motor.DJI_3508_Chassis[2].DATA, DJI_Motor_Resolve,  MOTOR_OFFLINE_TIME,   CHASSIS},
    {FDCAN1, 0x204, &All_Motor.DJI_3508_Chassis[3].DATA, DJI_Motor_Resolve,  MOTOR_OFFLINE_TIME,   CHASSIS},
    {FDCAN1, 0x206, &All_Motor.DJI_6020_Pitch.DATA,      DJI_Motor_Resolve,  MOTOR_OFFLINE_TIME,   GIMBAL},

    /* ----- FDCAN2 ----- */
    {FDCAN2, 0x205, &All_Motor.DJI_6020_Steer[0].DATA,   DJI_Motor_Resolve,  MOTOR_OFFLINE_TIME,   CHASSIS},
    {FDCAN2, 0x206, &All_Motor.DJI_6020_Steer[1].DATA,   DJI_Motor_Resolve,  MOTOR_OFFLINE_TIME,   CHASSIS},
    {FDCAN2, 0x207, &All_Motor.DJI_6020_Steer[2].DATA,   DJI_Motor_Resolve,  MOTOR_OFFLINE_TIME,   CHASSIS},
    {FDCAN2, 0x208, &All_Motor.DJI_6020_Steer[3].DATA,   DJI_Motor_Resolve,  MOTOR_OFFLINE_TIME,   CHASSIS},

    /* ----- FDCAN3 ----- */
    {FDCAN2, 0x301, &All_Motor.DM4310_Feed.DATA,         DM_1to4_Resolve,    MOTOR_OFFLINE_TIME,   SHOOT},
    {FDCAN3, 0x202, &All_Motor.DJI_3508_Pull.DATA,       DJI_Motor_Resolve,  MOTOR_OFFLINE_TIME,   SHOOT},
    {FDCAN3, 0x203, &All_Motor.DJI_3508_Yaw.DATA,        DJI_Motor_Resolve,  MOTOR_OFFLINE_TIME,   GIMBAL},
    {FDCAN3, 0x288, &cap.get,                     Power_Cap_Rx,       CAP_OFFLINE_TIME,         GROUP_NONE},
};

static const UART_Rx_Route_t UART_Rx_Config_Table[] = {
    // 总线    预期长度   主Buffer           备用Buffer(双缓冲)   DMA重载长度              目标结构体指针   解析函数                 超时    所属分组
    {UART5,  18,DBUS_RX_DATA,      NULL,               18,                     &DBUS,DBUS_Resolved,    DBUS_OFFLINE_TIME,   GROUP_NONE},
    {UART7,  21,VT13_RX_DATA,      NULL,               21,                     &VT13,VT13_Resolved,    DBUS_OFFLINE_TIME,   GROUP_NONE},
    {USART1, 0, Referee_Rx_Buf[0], Referee_Rx_Buf[1],  REFEREE_RXFRAME_LENGTH, NULL, Referee_System_Frame_Update, 100,  GROUP_NONE},
};

void MY_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    if (htim->Instance == TIM4) {
        WS2812_UpdateBreathing(0,1.5f);
        WS2812_Send();
        DWT_SysTimeUpdate();
        Offline_Monitor();
    }
}

void CAN_Hash_Table_Init(void)
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
            (BSP_CAN_Callback_t)CAN_Rx_Config_Table[i].resolve // 类型强转，抹平应用层函数类型
        );
    }
}

void UART_App_Rx_Dispatch(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size)
{
    size_t table_size = sizeof(UART_Rx_Config_Table) / sizeof(UART_Rx_Route_t);
    for (size_t i = 0; i < table_size; i++)
    {
        if (huart->Instance == UART_Rx_Config_Table[i].instance)
        {
            uint8_t *next_buf = UART_Rx_Config_Table[i].rx_buf0;
            if (UART_Rx_Config_Table[i].rx_buf1 != NULL) {
                next_buf = (pData == UART_Rx_Config_Table[i].rx_buf0) ?
                           UART_Rx_Config_Table[i].rx_buf1 :
                           UART_Rx_Config_Table[i].rx_buf0;
            }
            UART_ReceiveToIdle_DMA(huart, next_buf, UART_Rx_Config_Table[i].dma_rx_size);
            if (UART_Rx_Config_Table[i].expected_size != 0 && Size != UART_Rx_Config_Table[i].expected_size) {
                return;
            }
            if (UART_Rx_Config_Table[i].resolve != NULL) {
                UART_Rx_Config_Table[i].resolve(pData, UART_Rx_Config_Table[i].device_ptr, Size);
            }
            return;
        }
    }
}

void UART_App_Error_Dispatch(UART_HandleTypeDef *huart)
{
    size_t table_size = sizeof(UART_Rx_Config_Table) / sizeof(UART_Rx_Route_t);

    for (size_t i = 0; i < table_size; i++)
    {
        if (huart->Instance == UART_Rx_Config_Table[i].instance)
        {
            UART_ReceiveToIdle_DMA(huart, UART_Rx_Config_Table[i].rx_buf0, UART_Rx_Config_Table[i].dma_rx_size);
            return;
        }
    }
}

void Offline_Monitor(void)
{
    uint32_t now = HAL_GetTick();
    // 监测 CAN 设备表
    size_t can_table_size = sizeof(CAN_Rx_Config_Table) / sizeof(CAN_Rx_Route_t);
    for (size_t i = 0; i < can_table_size; i++) {
        Offline_Check_t *dev = (Offline_Check_t *)CAN_Rx_Config_Table[i].device_ptr;
        // 如果指针为空，或者超时阈值设为0（表示不需要检测），则跳过
        if (dev == NULL || CAN_Rx_Config_Table[i].timeout_ms == 0) continue;

        if ((now - dev->last_feed_tick) > CAN_Rx_Config_Table[i].timeout_ms) {
            dev->is_online = false;
        } else {
            dev->is_online = true;
        }
    }
    // 监测 UART 设备表
    size_t uart_table_size = sizeof(UART_Rx_Config_Table) / sizeof(UART_Rx_Route_t);
    for (size_t i = 0; i < uart_table_size; i++) {
        Offline_Check_t *dev = (Offline_Check_t *)UART_Rx_Config_Table[i].device_ptr;
        if (dev == NULL || UART_Rx_Config_Table[i].timeout_ms == 0) continue;

        if ((now - dev->last_feed_tick) > UART_Rx_Config_Table[i].timeout_ms) {
            dev->is_online = false;
        } else {
            dev->is_online = true;
        }
    }
}

bool Is_Group_Online(Device_Group_e group)
{
    // 检查 CAN 设备分组
    size_t can_table_size = sizeof(CAN_Rx_Config_Table) / sizeof(CAN_Rx_Route_t);
    for (size_t i = 0; i < can_table_size; i++) {
        if (group == GROUP_ALL || CAN_Rx_Config_Table[i].group == group) {
            Offline_Check_t *dev = (Offline_Check_t *)CAN_Rx_Config_Table[i].device_ptr;
            if (dev != NULL && dev->is_online == false) {
                return false;
            }
        }
    }
    // 检查 UART 设备分组
    size_t uart_table_size = sizeof(UART_Rx_Config_Table) / sizeof(UART_Rx_Route_t);
    for (size_t i = 0; i < uart_table_size; i++) {
        if (group == GROUP_ALL || UART_Rx_Config_Table[i].group == group) {
            Offline_Check_t *dev = (Offline_Check_t *)UART_Rx_Config_Table[i].device_ptr;
            if (dev != NULL && dev->is_online == false) {
                return false;
            }
        }
    }
    return true;
}