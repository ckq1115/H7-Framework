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
    PWM_WS2812 = 0,     // WS2812 灯组 (如 TIM3_CH2)
    PWM_BUZZER,         // 蜂鸣器 (如 TIM4_CH3)
    PWM_TEMP_CTRL,      // 物理温控加热片 (如 TIM12_CH1)
    PWM_SERVO_0,        // 舵机接口 0 (如 TIM5_CH1)
    PWM_SERVO_1,        // 舵机接口 1 (如 TIM5_CH2)

    PWM_DEVICE_CNT      // 标记总数
} PWM_Device_e;

/* 初始化全车所有的定时器 PWM 通道 */
void TIM_PWM_Init(void);
/* 通用设置占空比（通过比较值 CCR） */
void TIM_Set_Compare(PWM_Device_e device, uint32_t compare);
/* 通用设置频率（通过修改自动重装载值 ARR，主要用于蜂鸣器变调） */
void TIM_Set_Autoreload(PWM_Device_e device, uint32_t autoreload);

void WS2812_DMA_Handler(uint8_t half_cplt);

#endif //H7_FRAMEWORK_BSP_TIM_H
