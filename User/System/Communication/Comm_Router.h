//
// Created by CaoKangqi on 2026/6/17.
//

#ifndef H7_FRAMEWORK_INTERRUPT_H
#define H7_FRAMEWORK_INTERRUPT_H

#include <stdint.h>
#include "stm32h723xx.h"
#include "main.h"


// CAN 解析函数指针类型
typedef void (*CAN_Resolve_Func_t)(void *device_ptr, uint8_t *data);
// UART 解析函数指针类型
typedef void (*UART_Resolve_Func_t)(uint8_t *data, void *device_ptr, uint16_t size);

// CAN 路由表条目 (已剔除 offline 逻辑)
typedef struct {
    FDCAN_GlobalTypeDef *instance; // 对应的硬件总线
    uint32_t id;                   // 绑定的 CAN ID
    void *device_ptr;              // 对应的应用层变量指针
    CAN_Resolve_Func_t resolve;    // 对应的解析函数
} CAN_Rx_Route_t;

// UART 路由表条目 (已剔除 offline 逻辑)
typedef struct {
    UART_HandleTypeDef *huart;        // 对应的硬件总线
    uint16_t expected_size;         // 期望接收的字节数
    uint8_t *rx_buf0;               // 主接收缓冲区
    uint8_t *rx_buf1;               // 备用接收缓冲区
    uint16_t dma_rx_size;           // 每次重启 DMA 时请求的长度
    void *device_ptr;               // 对应的应用层变量指针
    UART_Resolve_Func_t resolve;    // 对应的解析函数
} UART_Rx_Route_t;

// 函数声明
void CAN_Router_Init(void);
void UART_Router_Init(void);

void MY_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim);

#endif //H7_FRAMEWORK_INTERRUPT_H
