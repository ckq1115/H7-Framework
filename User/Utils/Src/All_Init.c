//
// Created by CaoKangqi on 2026/2/13.
//
#include "All_Init.h"

//DBUS
uint8_t DBUS_RX_DATA[18]__attribute__((section(".RAM_D2")));
DBUS_Typedef DBUS = { 0 };

//VT13
uint8_t VT13_RX_DATA[21];
VT13_Typedef VT13 = { 0 };

//裁判系统
User_Data_T User_data;
uint8_t Referee_Rx_Buf[2][REFEREE_RXFRAME_LENGTH];

MOTOR_Typdef All_Motor;

//Offline_check


uint32_t stm32_id[3];
void Get_UID(uint32_t *uid) {
    uid[0] = HAL_GetUIDw0();
    uid[1] = HAL_GetUIDw1();
    uid[2] = HAL_GetUIDw2();
}
void All_Init() {
    DWT_Init(550);
    Get_UID(stm32_id);

    MX_USB_DEVICE_Init();
    UART_ReceiveToIdle_DMA(&huart5,DBUS_RX_DATA,18);//DBUS串口
    UART_ReceiveToIdle_DMA(&huart7, VT13_RX_DATA, 21);//图传链路串口
    UART_ReceiveToIdle_DMA(&huart1, Referee_Rx_Buf[0], REFEREE_RXFRAME_LENGTH);//裁判系统串口

    FDCAN_Config(&hfdcan1, FDCAN_RX_FIFO0);
    FDCAN_Config(&hfdcan2, FDCAN_RX_FIFO1);
    FDCAN_Config(&hfdcan3, FDCAN_RX_FIFO0);
    CAN_Hash_Table_Init();
    WS2812_Init();
    BMI088_init();


    HAL_TIM_Base_Start_IT(&htim4);
    HAL_TIMEx_PWMN_Start(&htim8, TIM_CHANNEL_3);//IMU加热
    HAL_TIM_PWM_Start(&htim12, TIM_CHANNEL_2);//蜂鸣器

    WS2812_SetPixel(0, 0, 0, 200);
    __HAL_TIM_SET_COMPARE(&htim12, TIM_CHANNEL_2, 100);
    HAL_Delay(500);
    __HAL_TIM_SET_COMPARE(&htim12, TIM_CHANNEL_2, 0);
}