//
// Created by CaoKangqi on 2026/6/19.
//

#ifndef H7_FRAMEWORK_BSP_TIM_H
#define H7_FRAMEWORK_BSP_TIM_H

#include "tim.h"

/**
 * @brief 全车 PWM 硬件资源枚举（将硬件彻底标签化）
 */
typedef enum {
    PWM_WS2812 = 0,     // WS2812
    PWM_BUZZER,         // 蜂鸣器
    PWM_TEMP_CTRL,      // IMU温控加热
    PWM_DEVICE_CNT      // 标记总数
} PWM_Device_e;

/* 初始化全车所有的定时器 PWM 通道 */
void TIM_PWM_Init(void);
/* 通用设置占空比（通过比较值 CCR） */
void TIM_Set_Compare(PWM_Device_e device, uint32_t compare);
/* 通用设置频率（通过修改自动重装载值 ARR，主要用于蜂鸣器变调） */
void TIM_Set_Autoreload(PWM_Device_e device, uint32_t autoreload);
void TIM_Set_Autoreload_Immediate(PWM_Device_e device, uint32_t autoreload, uint32_t compare);
void WS2812_DMA_Handler(uint8_t half_cplt);

#endif //H7_FRAMEWORK_BSP_TIM_H
