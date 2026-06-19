//
// Created by CaoKangqi on 2026/6/19.
//

#ifndef H7_FRAMEWORK_WS2812_H
#define H7_FRAMEWORK_WS2812_H

#include <stdint.h>

#define WS2812_PWM_LOW      100
#define WS2812_PWM_HIGH     235

#define MAX_LED             1    // 车载 LED 灯珠总数
#define WS2812_RESET_SLOTS  20   // Reset 信号所需的低电平双缓冲周期数量

typedef struct {
    uint8_t R;
    uint8_t G;
    uint8_t B;
} WS2812_Color_t;

/**
 * @brief 初始化灯组逻辑状态
 */
void WS2812_Init(void);

/**
 * @brief 设置指定灯珠的物理底色
 */
void WS2812_SetPixel(uint16_t index, uint8_t r, uint8_t g, uint8_t b);

/**
 * @brief 全车灯珠统一设色
 */
void WS2812_SetAll(uint8_t r, uint8_t g, uint8_t b);

/**
 * @brief 清空当前渲染帧缓存（全黑）
 */
void WS2812_Clear(void);

/**
 * @brief 触发双缓冲 DMA 将数据推向硬件喷射
 */
void WS2812_Send(void);

/**
 * @brief 周期性调用：基于底色和正弦表更新呼吸特效
 * @param period 呼吸总周期（秒）
 */
void WS2812_UpdateBreathing(uint16_t index, float period);

/**
 * @brief 周期性调用：基于底色进行闪烁警报特效
 * @param interval_s 亮灭翻转的时间间隔（秒）
 */
void WS2812_Blink(uint16_t index, float interval_s);

#endif //H7_FRAMEWORK_WS2812_H