//
// Created by CaoKangqi on 2026/6/10.
//

#ifndef H7_FRAMEWORK_BSP_UART_H
#define H7_FRAMEWORK_BSP_UART_H

#include "stm32h7xx_hal_uart.h"

// 统一的底层回调函数指针类型：只抛出接收数据的指针、实际长度，不涉及任何应用层特有结构体
typedef void (*BSP_UART_Callback_t)(uint8_t *pData, void *device_ptr, uint16_t Size);

typedef struct {
    uint8_t *rx_buf0;
    uint8_t *rx_buf1;
    uint16_t dma_rx_size;
    uint16_t expected_size;
    void *device_ptr;               // 摆渡指针，底层只负责保管，不关心它是啥
    BSP_UART_Callback_t resolve;
} BSP_UART_Slot_t;

// 严格按照你在 Comm_Router 里调用的参数顺序声明
void BSP_UART_Register_Slot(UART_HandleTypeDef *huart,
                            uint16_t expected_size,
                            uint8_t *rx_buf0,
                            uint8_t *rx_buf1,
                            uint16_t dma_size,
                            void *device_ptr,
                            BSP_UART_Callback_t callback);

HAL_StatusTypeDef UART_ReceiveToIdle_DMA(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size);

#endif //H7_FRAMEWORK_BSP_UART_H