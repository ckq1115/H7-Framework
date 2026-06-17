//
// Created by CaoKangqi on 2026/6/17.
//
#include "Interrupt.h"

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

static const CAN_Rx_Route_t CAN_Rx_Config_Table[] = {
    /* ----- FDCAN1 ----- */
    {FDCAN1, 0x201, &All_Motor.DJI_3508_Chassis[0], DJI_Motor_Resolve},
    {FDCAN1, 0x202, &All_Motor.DJI_3508_Chassis[1], DJI_Motor_Resolve},
    {FDCAN1, 0x203, &All_Motor.DJI_3508_Chassis[2], DJI_Motor_Resolve},
    {FDCAN1, 0x204, &All_Motor.DJI_3508_Chassis[3], DJI_Motor_Resolve},
    {FDCAN1, 0x206, &All_Motor.DJI_6020_Pitch,      DJI_Motor_Resolve},

    /* ----- FDCAN2 ----- */
    {FDCAN2, 0x301, &All_Motor.DM4310_Feed,         DM_1to4_Resolve},
    {FDCAN2, 0x202, &All_Motor.DJI_3508_Pull,       DJI_Motor_Resolve},
    {FDCAN2, 0x203, &All_Motor.DJI_3508_Yaw,        DJI_Motor_Resolve},
    {FDCAN2, 0x288, &cap,                    Power_Cap_Rx},// 超级电容

    /* ----- FDCAN3 ----- */
    {FDCAN3, 0x205, &All_Motor.DJI_6020_Steer[0],   DJI_Motor_Resolve},
    {FDCAN3, 0x206, &All_Motor.DJI_6020_Steer[1],   DJI_Motor_Resolve},
    {FDCAN3, 0x207, &All_Motor.DJI_6020_Steer[2],   DJI_Motor_Resolve},
    {FDCAN3, 0x208, &All_Motor.DJI_6020_Steer[3],   DJI_Motor_Resolve},
};

void CAN_App_Frame_Dispatch(FDCAN_HandleTypeDef *hfdcan, uint32_t identifier, uint8_t *data, uint32_t len)
{
    (void)len; // 对齐接口后，长度信息在包装函数内处理或忽略
    size_t table_size = sizeof(CAN_Rx_Config_Table) / sizeof(CAN_Rx_Route_t);
    for (size_t i = 0; i < table_size; i++)
    {
        if ((hfdcan->Instance == CAN_Rx_Config_Table[i].instance) &&
            (identifier == CAN_Rx_Config_Table[i].id))
        {
            if (CAN_Rx_Config_Table[i].resolve != NULL)
            {
                CAN_Rx_Config_Table[i].resolve(CAN_Rx_Config_Table[i].device_ptr, data);
            }
            return;
        }
    }
}

void MY_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    if (htim->Instance == TIM4) {
        WS2812_UpdateBreathing(0, 2.0f);
        WS2812_Send();
        DWT_SysTimeUpdate();
    }
}