//
// Created by CaoKangqi on 2026/2/13.
//
#include "System_Init.h"

#include "All_Motor.h"
#include "BSP_DWT.h"
#include "BSP_FDCAN.h"
#include "WS2812.h"
#include "BMI088driver.h"
#include "BSP_TIM.h"
#include "Comm_Router.h"
#include "System_State.h"

uint32_t stm32_id[3];
void Get_UID(uint32_t *uid) {
    uid[0] = HAL_GetUIDw0();
    uid[1] = HAL_GetUIDw1();
    uid[2] = HAL_GetUIDw2();
}
void System_Init() {
    DWT_Init(550);
    Get_UID(stm32_id);

    FDCAN_Config(&hfdcan1, FDCAN_RX_FIFO0);
    FDCAN_Config(&hfdcan2, FDCAN_RX_FIFO1);
    FDCAN_Config(&hfdcan3, FDCAN_RX_FIFO0);
    CAN_Router_Init();
    UART_Router_Init();

    WS2812_Init();
    BMI088_init();

    HAL_TIM_Base_Start_IT(&htim4);
    TIM_PWM_Init();

    WS2812_SetPixel(0, 0, 0, 200);
    System_State_Init();
    __HAL_TIM_SET_COMPARE(&htim12, TIM_CHANNEL_2, 100);
    HAL_Delay(500);
    __HAL_TIM_SET_COMPARE(&htim12, TIM_CHANNEL_2, 0);
}