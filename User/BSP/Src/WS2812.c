//
// Created by CaoKangqi on 2026/1/23.
//
#include "WS2812.h"
#include <string.h>
#include "BSP_DWT.h"
#include "tim.h"

// 0-255 的正弦波表 (0 -> 255 -> 0)，用于呼吸灯，避免 sinf 计算
const uint8_t Sine_Table[256] = {
    0,   0,   0,   0,   1,   1,   1,   2,   2,   3,   4,   5,   6,   7,   8,   9,
    11,  12,  13,  15,  17,  18,  20,  22,  24,  26,  28,  30,  32,  35,  37,  39,
    42,  44,  47,  49,  52,  55,  58,  60,  63,  66,  69,  72,  75,  78,  81,  85,
    88,  91,  94,  97,  101, 104, 107, 111, 114, 117, 121, 124, 127, 131, 134, 137,
    141, 144, 147, 150, 154, 157, 160, 163, 167, 170, 173, 176, 179, 182, 185, 188,
    191, 194, 197, 200, 202, 205, 208, 210, 213, 215, 217, 220, 222, 224, 226, 229,
    231, 232, 234, 236, 238, 239, 241, 242, 244, 245, 246, 248, 249, 250, 251, 251,
    252, 253, 253, 254, 254, 255, 255, 255, 255, 255, 255, 255, 254, 254, 253, 253,
    252, 251, 251, 250, 249, 248, 246, 245, 244, 242, 241, 239, 238, 236, 234, 232,
    231, 229, 226, 224, 222, 220, 217, 215, 213, 210, 208, 205, 202, 200, 197, 194,
    191, 188, 185, 182, 179, 176, 173, 170, 167, 163, 160, 157, 154, 150, 147, 144,
    141, 137, 134, 131, 127, 124, 121, 117, 114, 111, 107, 104, 101, 97,  94,  91,
    88,  85,  81,  78,  75,  72,  69,  66,  63,  60,  58,  55,  52,  49,  47,  44,
    42,  39,  37,  35,  32,  30,  28,  26,  24,  22,  20,  18,  17,  15,  13,  12,
    11,  9,   8,   7,   6,   5,   4,   3,   2,   2,   1,   1,   1,   0,   0,   0
};

// 逻辑颜色数据
static WS2812_Color_t Base_Color[MAX_LED];
static WS2812_Color_t LED_Data[MAX_LED];

// 双缓冲 DMA 发送数组
// 只包含 2 个 LED 的数据量 (24 bits * 2 = 48 words)
// 前 24 个是 Buffer_A，后 24 个是 Buffer_B
#define WS2812_DMA_BUF_LEN (24 * 2)
uint16_t DMA_Buffer[WS2812_DMA_BUF_LEN]__attribute__((section(".RAM_D1")));

static uint8_t Global_Brightness = 255;
static volatile uint8_t isSending = 0;
static uint16_t send_pixel_idx = 0; // 当前正在填充第几个像素的数据


/**
 * @brief 将一个 LED 的 RGB 数据填充到 DMA 缓冲区的指定位置
 * @param ledIdx: LED_Data 数组中的索引 (如果是复位阶段，则不用)
 * @param bufferOffset: DMA_Buffer 中的偏移 (0 或 24)
 * @param isReset: 是否发送复位信号(0占空比)
 */
static void Fill_Buffer(uint16_t ledIdx, uint16_t bufferOffset, uint8_t isReset) {
    if (isReset) {
        memset(&DMA_Buffer[bufferOffset], 0, 24 * sizeof(uint16_t));
        return;
    }
    // 获取颜色并计算全局亮度
    uint8_t r = LED_Data[ledIdx].R;
    uint8_t g = LED_Data[ledIdx].G;
    uint8_t b = LED_Data[ledIdx].B;

    // 简单的位移计算亮度，比 float 快得多
    if (Global_Brightness < 255) {
        r = (r * Global_Brightness) >> 8;
        g = (g * Global_Brightness) >> 8;
        b = (b * Global_Brightness) >> 8;
    }
    // 组合颜色 GRB
    uint32_t color = ((uint32_t)g << 16) | ((uint32_t)r << 8) | (uint32_t)b;
    // 填充 24 位 PWM 数据
    for (int8_t i = 23; i >= 0; i--) {
        DMA_Buffer[bufferOffset++] = (color & (1 << i)) ? WS2812_PWM_HIGH : WS2812_PWM_LOW;
    }
}

void WS2812_Init(void) {
    isSending = 0;
    WS2812_Clear();
}

void WS2812_SetPixel(uint16_t index, uint8_t r, uint8_t g, uint8_t b) {
    if (index >= MAX_LED) return;
    Base_Color[index].R = r;
    Base_Color[index].G = g;
    Base_Color[index].B = b;

    LED_Data[index] = Base_Color[index];
}

void WS2812_SetAll(uint8_t r, uint8_t g, uint8_t b) {
    for (uint16_t i = 0; i < MAX_LED; i++) {
        WS2812_SetPixel(i, r, g, b);
    }
}

void WS2812_Clear(void) {
    memset(LED_Data, 0, sizeof(LED_Data));
}

void WS2812_Send(void) {
    if (isSending) return;
    send_pixel_idx = 0;

    Fill_Buffer(0, 0, 0);
    if (MAX_LED > 1) {
        Fill_Buffer(1, 24, 0);
        send_pixel_idx = 2;
    } else {
        Fill_Buffer(0, 24, 1); // 第二个半周填复位信号
        send_pixel_idx = 1;
    }

    isSending = 1;
    HAL_TIM_PWM_Start_DMA(&WS2812_TIM_HANDLE, WS2812_TIM_CHANNEL, (uint32_t *)DMA_Buffer, WS2812_DMA_BUF_LEN);
}


// 半传输完成 (Half Transfer) -> 刚刚发完了 Buffer 的前半部分 (0-23)
// 此时 DMA 正在发后半部分，CPU 赶紧去填充前半部分
void HAL_TIM_PWM_PulseFinishedHalfCpltCallback(TIM_HandleTypeDef *htim) {
    if (htim->Instance != WS2812_TIM_HANDLE.Instance) return;
    // 如果还有 LED 数据没发完
    if (send_pixel_idx < MAX_LED) {
        Fill_Buffer(send_pixel_idx, 0, 0); // 填数据到前半段
        send_pixel_idx++;
    }
    // 如果 LED 数据发完了，但是需要发 Reset 信号 (50us 的低电平)
    else if (send_pixel_idx < MAX_LED + WS2812_RESET_SLOTS) {
        Fill_Buffer(0, 0, 1); // 填 0 到前半段
        send_pixel_idx++;
    }
    // 全发完了，不需要在这里停，通常在 TC (全传输) 里停比较整齐，或者让它跑完 reset
}

// 全传输完成 (Transfer Complete) -> 刚刚发完了 Buffer 的后半部分 (24-47)
// 此时 DMA 回滚去发前半部分，CPU 赶紧去填充后半部分
void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim) {
    if (htim->Instance != WS2812_TIM_HANDLE.Instance) return;
    // 如果还有 LED 数据没发完
    if (send_pixel_idx < MAX_LED) {
        Fill_Buffer(send_pixel_idx, 24, 0); // 填数据到后半段
        send_pixel_idx++;
    }
    // 发 Reset 信号
    else if (send_pixel_idx < MAX_LED + WS2812_RESET_SLOTS) {
        Fill_Buffer(0, 24, 1); // 填 0 到后半段
        send_pixel_idx++;
    }
    // Reset 信号也发够了，停止发送
    else {
        HAL_TIM_PWM_Stop_DMA(&WS2812_TIM_HANDLE, WS2812_TIM_CHANNEL);
        isSending = 0;
    }
    __HAL_TIM_SET_COMPARE(&WS2812_TIM_HANDLE, WS2812_TIM_CHANNEL, 0);
}


/**
 * @brief  让指定灯珠按底色进行呼吸
 * @param  index:  LED 索引
 * @param  period: 呼吸周期 (秒)
 */
void WS2812_UpdateBreathing(uint16_t index, float period) {
    if (index >= MAX_LED) return;
    uint16_t period_ms = (uint16_t)(period * 1000);
    uint32_t now = HAL_GetTick();
    // 将周期映射到 0-255 的查表索引
    uint32_t idx = (now % period_ms) * 255 / period_ms;
    uint32_t factor = Sine_Table[idx];

    // 整数位移代替浮点缩放
    LED_Data[index].R = (Base_Color[index].R * factor) >> 8;
    LED_Data[index].G = (Base_Color[index].G * factor) >> 8;
    LED_Data[index].B = (Base_Color[index].B * factor) >> 8;
}

// 无浮点 HSV 转 RGB
// h: 0-255, s: 0-255, v: 0-255
void WS2812_SetHSV(uint16_t index, uint8_t h, uint8_t s, uint8_t v) {
    uint8_t r, g, b;
    uint8_t region, remainder, p, q, t;

    if (s == 0) {
        r = v; g = v; b = v;
    } else {
        region = h / 43;
        remainder = (h - (region * 43)) * 6;

        p = (v * (255 - s)) >> 8;
        q = (v * (255 - ((s * remainder) >> 8))) >> 8;
        t = (v * (255 - ((s * (255 - remainder)) >> 8))) >> 8;

        switch (region) {
            case 0: r = v; g = t; b = p; break;
            case 1: r = q; g = v; b = p; break;
            case 2: r = p; g = v; b = t; break;
            case 3: r = p; g = q; b = v; break;
            case 4: r = t; g = p; b = v; break;
            default: r = v; g = p; b = q; break;
        }
    }
    WS2812_SetPixel(index, r, g, b);
}

/**
 * @brief  全灯幻彩变换
 * @param  interval_ms: 多少毫秒更新一次色相 (值越大越慢)
 */
void WS2812_RainbowCycle(uint16_t interval_ms) {
    static uint32_t last_update_time = 0;
    static uint8_t start_hue = 0;

    // 时间没到就直接退出，不影响 main 循环其他逻辑
    if (HAL_GetTick() - last_update_time < interval_ms) {
        return;
    }
    last_update_time = HAL_GetTick();

    for (uint16_t i = 0; i < MAX_LED; i++) {
        // 让彩虹在灯条上分布开
        uint8_t hue = (start_hue + (i * 256 / MAX_LED)) % 256;
        WS2812_SetHSV(i, hue, 255, 255);
    }

    start_hue++; // 步进始终为 1，保证色彩过渡平滑
}
/**
 * @brief  流水灯效果
 * @param  r, g, b: 流水灯的主颜色
 * @param  speed: 移动速度 (ms/格)
 */
void WS2812_WaterFlow(uint8_t r, uint8_t g, uint8_t b, uint8_t speed) {
    static uint32_t last_time = 0;
    static uint16_t head = 0;

    if (HAL_GetTick() - last_time < speed) return;
    last_time = HAL_GetTick();

    // 1. 先让所有灯珠的亮度衰减（产生拖尾感）
    for (uint16_t i = 0; i < MAX_LED; i++) {
        LED_Data[i].R = LED_Data[i].R * 0.7; // 衰减系数 0.7
        LED_Data[i].G = LED_Data[i].G * 0.7;
        LED_Data[i].B = LED_Data[i].B * 0.7;
    }

    // 2. 更新“头”的位置并赋全色
    WS2812_SetPixel(head, r, g, b);

    head = (head + 1) % MAX_LED;
}