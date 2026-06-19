//
// Created by CaoKangqi on 2026/1/23.
//
#include "WS2812.h"
#include "BSP_TIM.h"
#include <string.h>

// 0-255 的无浮点正弦波表 (0 -> 255 -> 0)
static const uint8_t Sine_Table[256] = {
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

// 逻辑表现层颜色看板
static WS2812_Color_t Base_Color[MAX_LED];
static WS2812_Color_t LED_Data[MAX_LED];

// 乒乓双缓冲大小 (24 bits * 2 空间 = 48 字节)
#define WS2812_DMA_BUF_LEN (24 * 2)

__attribute__((section(".RAM_D2"))) uint16_t DMA_Buffer[WS2812_DMA_BUF_LEN];

static uint8_t Global_Brightness = 255;
static volatile uint8_t isSending = 0;
static uint16_t send_pixel_idx = 0;


/**
 * @brief 将一个 LED 的 RGB 数据填充到 DMA 缓冲区的指定位置
 * @param ledIdx: LED_Data 数组中的索引 (如果是复位阶段，则不用)
 * @param bufferOffset: DMA_Buffer 中的偏移 (0 或 24)
 * @param isReset: 是否发送复位信号(0占空比)
 */
static void Fill_Buffer(uint16_t ledIdx, uint16_t bufferOffset, uint8_t isReset)
{
    if (isReset) {
        memset(&DMA_Buffer[bufferOffset], 0, 24 * sizeof(uint16_t));
        return;
    }

    uint8_t r = LED_Data[ledIdx].R;
    uint8_t g = LED_Data[ledIdx].G;
    uint8_t b = LED_Data[ledIdx].B;

    // 高效整数位移执行全局亮度微调
    if (Global_Brightness < 255) {
        r = (r * Global_Brightness) >> 8;
        g = (g * Global_Brightness) >> 8;
        b = (b * Global_Brightness) >> 8;
    }

    // 拼装标准 WS2812 所要求的 G->R->B 串行流
    uint32_t color = ((uint32_t)g << 16) | ((uint32_t)r << 8) | (uint32_t)b;

    for (int8_t i = 23; i >= 0; i--) {
        DMA_Buffer[bufferOffset++] = (color & (1 << i)) ? WS2812_PWM_HIGH : WS2812_PWM_LOW;
    }
}

void WS2812_Init(void)
{
    isSending = 0;
    WS2812_Clear();
}

void WS2812_SetPixel(uint16_t index, uint8_t r, uint8_t g, uint8_t b)
{
    if (index >= MAX_LED) return;
    Base_Color[index].R = r;
    Base_Color[index].G = g;
    Base_Color[index].B = b;
    LED_Data[index] = Base_Color[index];
}

void WS2812_SetAll(uint8_t r, uint8_t g, uint8_t b)
{
    for (uint16_t i = 0; i < MAX_LED; i++) {
        WS2812_SetPixel(i, r, g, b);
    }
}

void WS2812_Clear(void)
{
    memset(LED_Data, 0, sizeof(LED_Data));
}

void WS2812_Send(void)
{
    if (isSending) return;
    send_pixel_idx = 0;

    // 预填初阶双缓冲：Buffer_A 填第一颗灯，Buffer_B 视总数填第二颗或 Reset
    Fill_Buffer(0, 0, 0);
    if (MAX_LED > 1) {
        Fill_Buffer(1, 24, 0);
        send_pixel_idx = 2;
    } else {
        Fill_Buffer(0, 24, 1);
        send_pixel_idx = 1;
    }

    isSending = 1;

    // 通过获取底层硬件资源安全开启 DMA 搬运
    HAL_TIM_PWM_Start_DMA(&htim3, TIM_CHANNEL_2, (uint32_t *)DMA_Buffer, WS2812_DMA_BUF_LEN);
}


void WS2812_DMA_Handler(uint8_t half_cplt)
{
    if (half_cplt == 1)
    {
        /* ---------- 原 HalfCpltCallback 逻辑 (填前半段 0 偏移) ---------- */
        if (send_pixel_idx < MAX_LED) {
            Fill_Buffer(send_pixel_idx, 0, 0);
            send_pixel_idx++;
        }
        else if (send_pixel_idx < MAX_LED + WS2812_RESET_SLOTS) {
            Fill_Buffer(0, 0, 1);
            send_pixel_idx++;
        }
    }
    else
    {
        /* ---------- 原 PulseFinishedCallback 逻辑 (填后半段 24 偏移) ---------- */
        if (send_pixel_idx < MAX_LED) {
            Fill_Buffer(send_pixel_idx, 24, 0);
            send_pixel_idx++;
        }
        else if (send_pixel_idx < MAX_LED + WS2812_RESET_SLOTS) {
            Fill_Buffer(0, 24, 1);
            send_pixel_idx++;
        }
        else {
            // 全部喷射完毕，通知统一的 TIM 驱动把自身占空比强行归零，防止浮空产生错位闪烁
            HAL_TIM_PWM_Stop_DMA(&htim3, TIM_CHANNEL_2); // 仅保留必要的DMA停止句柄
            isSending = 0;

            TIM_Set_Compare(PWM_WS2812, 0);
        }
    }
}


/**
 * @brief  让指定灯珠按底色进行呼吸
 * @param  index:  LED 索引
 * @param  period: 呼吸周期 (秒)
 */
void WS2812_UpdateBreathing(uint16_t index, float period)
{
    if (index >= MAX_LED) return;
    uint16_t period_ms = (uint16_t)(period * 1000);
    uint32_t now = HAL_GetTick();

    uint32_t idx = (now % period_ms) * 255 / period_ms;
    uint32_t factor = Sine_Table[idx];

    LED_Data[index].R = (Base_Color[index].R * factor) >> 8;
    LED_Data[index].G = (Base_Color[index].G * factor) >> 8;
    LED_Data[index].B = (Base_Color[index].B * factor) >> 8;
}

/**
 * @brief  让指定灯珠按底色进行闪烁
 * @param  index: LED 索引
 * @param  interval_s: 亮灭切换的时间间隔 (s)
 */
void WS2812_Blink(uint16_t index, float interval_s)
{
    if (index >= MAX_LED) return;

    static uint32_t last_tick = 0;
    static uint8_t is_on = 1;
    uint32_t now = HAL_GetTick();

    if (now - last_tick >= (uint32_t)(interval_s * 1000)) {
        last_tick = now;
        is_on = !is_on;
    }

    if (is_on) {
        LED_Data[index] = Base_Color[index];
    } else {
        LED_Data[index].R = 0;
        LED_Data[index].G = 0;
        LED_Data[index].B = 0;
    }
}