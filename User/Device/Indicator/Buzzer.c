//
// Created by CaoKangqi on 2026/6/21.
//
#include "Buzzer.h"
#include "BSP_TIM.h"

typedef enum {
    BUZZER_IDLE = 0,
    BUZZER_STATE_RUNNING
} Buzzer_Ctrl_State_e;

// 每个状态音效最多包含 6 个音符片段
#define MAX_EFFECT_FRAGMENTS 6

typedef struct {
    uint16_t freq;
    uint16_t duration;
} Tone_Fragment_t;

typedef struct {
    Buzzer_Ctrl_State_e state;
    uint32_t timer_ms;
    Tone_Fragment_t effect[MAX_EFFECT_FRAGMENTS];
    uint8_t total_fragments;
    uint8_t current_fragment;
} Buzzer_Ctrl_t;

static Buzzer_Ctrl_t buzzer;

/**
 * @brief 底层频率控制
 */
static void Buzzer_Set_Freq(uint16_t frequency_hz)
{
    if (frequency_hz == 0) {
        Buzzer_Off();
    } else {
        // 基于 TIM12 1MHz 计数时钟：ARR = 1000000 / 频率 - 1
        uint32_t arr = 1000000 / frequency_hz - 1;
        uint32_t ccr = (arr + 1) / 2; // 50% 占空比，保证音量
        TIM_Set_Autoreload_Immediate(PWM_BUZZER, arr, ccr);
    }
}

void Buzzer_Init(void)
{
    buzzer.state = BUZZER_IDLE;
    buzzer.timer_ms = 0;
    buzzer.total_fragments = 0;
    buzzer.current_fragment = 0;
    Buzzer_Off();
}

void Buzzer_Off(void)
{
    TIM_Set_Compare(PWM_BUZZER, 0);
}

/**
 * @brief 传统的单音爆发接口（保留，方便应急使用）
 */
void Buzzer_Beep(uint16_t frequency_hz, uint32_t duration_ms)
{
    if (duration_ms == 0) return;

    buzzer.state = BUZZER_IDLE; // 打断当前状态音效
    Buzzer_Set_Freq(frequency_hz);
    buzzer.effect[0].freq = frequency_hz;
    buzzer.effect[0].duration = duration_ms;
    buzzer.effect[1].freq = 0; // 播完变安静
    buzzer.effect[1].duration = 1;

    buzzer.total_fragments = 2;
    buzzer.current_fragment = 0;
    buzzer.timer_ms = duration_ms;
    buzzer.state = BUZZER_STATE_RUNNING;
}

/**
 * @brief 触发状态提示音
 */
void Buzzer_Trigger_Status(Buzzer_Status_e status)
{
    // 如果当前正在处理严重错误报警，低级别的提示音无法打断它
    if (buzzer.state == BUZZER_STATE_RUNNING &&
        buzzer.effect[0].freq == 3000 &&
        status != BUZZER_STATUS_ERROR_CRITICAL) {
        return;
    }

    buzzer.current_fragment = 0;

    switch (status) {
        case BUZZER_STATUS_INIT_SUCCESS: // 1. 初始化成功
            buzzer.effect[0] = (Tone_Fragment_t){1500, 80};
            buzzer.effect[1] = (Tone_Fragment_t){0, 30};
            buzzer.effect[2] = (Tone_Fragment_t){2500, 120};
            buzzer.total_fragments = 3;
            break;

        case BUZZER_STATUS_CLICK_HINT:   // 2. 正常连接/校准成功
            buzzer.effect[0] = (Tone_Fragment_t){2500, 120};
            buzzer.effect[1] = (Tone_Fragment_t){0, 20};
            buzzer.effect[2] = (Tone_Fragment_t){3000, 180};
            buzzer.total_fragments = 3;
            break;

        case BUZZER_STATUS_WARN_DISCONNECT: // 3. 一般警告/掉线
            buzzer.effect[0] = (Tone_Fragment_t){1500, 200};
            buzzer.effect[1] = (Tone_Fragment_t){0, 50};
            buzzer.effect[2] = (Tone_Fragment_t){1500, 200};
            buzzer.total_fragments = 3;
            break;

        case BUZZER_STATUS_ERROR_CRITICAL:  // 4. 严重错误
            buzzer.effect[0] = (Tone_Fragment_t){3000, 80};
            buzzer.effect[1] = (Tone_Fragment_t){0, 50};
            buzzer.effect[2] = (Tone_Fragment_t){3000, 80};
            buzzer.effect[3] = (Tone_Fragment_t){0, 50};
            buzzer.effect[4] = (Tone_Fragment_t){3000, 80};
            buzzer.effect[5] = (Tone_Fragment_t){0, 50};
            buzzer.total_fragments = 6;
            break;
    }

    // 启动播放
    Buzzer_Set_Freq(buzzer.effect[0].freq);
    buzzer.timer_ms = buzzer.effect[0].duration;
    buzzer.state = BUZZER_STATE_RUNNING;
}

/**
 * @brief 蜂鸣器状态机心跳，1ms 严格调用一次
 */
void Buzzer_Ticks_1ms(void)
{
    if (buzzer.state == BUZZER_IDLE) {
        return;
    }

    // 倒计时未归零时，只负责递减并安全退出
    if (buzzer.timer_ms > 0) {
        buzzer.timer_ms--;
        return;
    }

    // 倒计时归零，处理下一段音效片段
    buzzer.current_fragment++;
    if (buzzer.current_fragment < buzzer.total_fragments) {
        Buzzer_Set_Freq(buzzer.effect[buzzer.current_fragment].freq);
        buzzer.timer_ms = buzzer.effect[buzzer.current_fragment].duration;

        if (buzzer.timer_ms == 0) buzzer.timer_ms = 1; // 防御性保护
    } else {
        // 整个效果片段播放完毕
        Buzzer_Off();
        buzzer.state = BUZZER_IDLE;
    }
}