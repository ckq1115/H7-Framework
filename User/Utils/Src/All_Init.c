//
// Created by CaoKangqi on 2026/2/13.
//
#include "All_Init.h"

uint32_t stm32_id[3];
void Get_UID(uint32_t *uid) {
    uid[0] = HAL_GetUIDw0();
    uid[1] = HAL_GetUIDw1();
    uid[2] = HAL_GetUIDw2();
}
void All_Init() {
    DWT_Init(550);
    Get_UID(stm32_id);


    FDCAN_Config(&hfdcan1, FDCAN_RX_FIFO0);
    FDCAN_Config(&hfdcan2, FDCAN_RX_FIFO1);
    FDCAN_Config(&hfdcan3, FDCAN_RX_FIFO0);

    WS2812_Init();
    HAL_TIM_Base_Start_IT(&htim4);

}