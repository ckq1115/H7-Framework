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
    uint32_t timeout_ms;           // 离线超时阈值
    Device_Group_e group;          // 所属分组
} CAN_Rx_Route_t;

typedef void (*UART_Resolve_Func_t)(uint8_t *data, void *device_ptr, uint16_t size);

void CAN_Hash_Table_Init(void);

typedef struct {
    USART_TypeDef *instance;        // 对应的硬件总线：UART5 / USART1 等
    uint16_t expected_size;         // 期望接收的字节数 (填 0 表示不限制，如不定长数据)
    uint8_t *rx_buf0;               // 主接收缓冲区
    uint8_t *rx_buf1;               // 备用接收缓冲区 (没有填 NULL)
    uint16_t dma_rx_size;           // 每次重启 DMA 时请求的长度
    void *device_ptr;               // 对应的应用层变量指针
    UART_Resolve_Func_t resolve;    // 对应的解析函数
    uint32_t timeout_ms;            // 离线超时阈值
    Device_Group_e group;           // 所属分组
} UART_Rx_Route_t;

void UART_App_Rx_Dispatch(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size);

void MY_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim);

#endif //H7_FRAMEWORK_INTERRUPT_H
