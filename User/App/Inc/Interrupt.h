//
// Created by CaoKangqi on 2026/6/17.
//

#ifndef H7_FRAMEWORK_INTERRUPT_H
#define H7_FRAMEWORK_INTERRUPT_H

#include "All_Init.h"

typedef void (*CAN_Resolve_Func_t)(void *instance, uint8_t *data);

// 路由表条目结构体
typedef struct {
    FDCAN_GlobalTypeDef *instance; // 对应的硬件总线：FDCAN1 / FDCAN2 / FDCAN3
    uint32_t id;                   // 绑定的 CAN ID
    void *device_ptr;              // 对应的应用层变量指针
    CAN_Resolve_Func_t resolve;    // 对应的中层协议解析函数
} CAN_Rx_Route_t;

void CAN_App_Frame_Dispatch(FDCAN_HandleTypeDef *hfdcan, uint32_t identifier, uint8_t *data, uint32_t len);
void MY_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim);

#endif //H7_FRAMEWORK_INTERRUPT_H
