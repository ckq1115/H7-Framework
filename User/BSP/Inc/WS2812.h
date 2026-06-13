//
// Created by CaoKangqi on 2026/1/23.
//

#ifndef H7_FRAMEWORK_WS2812_H
#define H7_FRAMEWORK_WS2812_H

#include <stdint.h>

#define WS2812_TIM_HANDLE htim3
#define WS2812_TIM_CHANNEL TIM_CHANNEL_2


#define WS2812_PWM_LOW  100
#define WS2812_PWM_HIGH 235

#define MAX_LED 1         // LED 灯珠总数
#define WS2812_RESET_SLOTS 20  // Reset 信号所需的低电平数量

typedef struct {
    uint8_t R;
    uint8_t G;
    uint8_t B;
} WS2812_Color_t;

void WS2812_Init(void);
void WS2812_SetPixel(uint16_t index, uint8_t r, uint8_t g, uint8_t b);
void WS2812_SetAll(uint8_t r, uint8_t g, uint8_t b);
void WS2812_Clear(void);
void WS2812_Send(void);

void WS2812_UpdateBreathing(uint16_t index, float period);
void WS2812_SetHSV(uint16_t index, uint8_t h, uint8_t s, uint8_t v);
void WS2812_RainbowCycle(uint16_t interval_ms);
void WS2812_WaterFlow(uint8_t r, uint8_t g, uint8_t b, uint8_t speed);

#endif //H7_FRAMEWORK_WS2812_H