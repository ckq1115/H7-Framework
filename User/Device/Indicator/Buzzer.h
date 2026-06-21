//
// Created by CaoKangqi on 2026/6/21.
//

#ifndef H7_FRAMEWORK_BUZZER_H
#define H7_FRAMEWORK_BUZZER_H

#include "stdint.h"

/**
 * @brief 机器人指示状态枚举
 */
typedef enum {
    BUZZER_STATUS_INIT_SUCCESS = 0,  // 开机/初始化成功
    BUZZER_STATUS_CLICK_HINT,        // 按键/正常操作提示音/校准成功
    BUZZER_STATUS_WARN_DISCONNECT,   // 警告：设备掉线（遥控器等）
    BUZZER_STATUS_ERROR_CRITICAL     // 严重错误：传感器损坏/电机堵转
} Buzzer_Status_e;

/* 初始化蜂鸣器 */
void Buzzer_Init(void);

/* 关闭蜂鸣器 */
void Buzzer_Off(void);

/* 基础单音爆发 */
void Buzzer_Beep(uint16_t frequency_hz, uint32_t duration_ms);

/* 触发状态提示音（非阻塞，高内聚） */
void Buzzer_Trigger_Status(Buzzer_Status_e status);

/* 核心心跳函数：挂载在 1ms 任务或中断中 */
void Buzzer_Ticks_1ms(void);

#endif //H7_FRAMEWORK_BUZZER_H
