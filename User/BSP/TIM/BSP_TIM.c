//
// Created by CaoKangqi on 2026/6/19.
//
#include "BSP_TIM.h"

typedef enum {
    CHANNEL_TYPE_NORMAL = 0, // 普通正向通道 (CH1, CH2, CH3, CH4)
    CHANNEL_TYPE_COMP,       // 高级定时器互补/反向通道 (CH1N, CH2N, CH3N)
} TIM_Channel_Type_e;

/**
 * @brief 内部硬件映射拓扑结构体
 */
typedef struct {
    TIM_HandleTypeDef *htim;   // HAL 库定时器句柄指针
    uint32_t channel;          // 定时器通道号
    TIM_Channel_Type_e type;   // 通道极性类型
} TIM_PWM_Map_t;

// 极其干净的全车硬件绑定表
static const TIM_PWM_Map_t PWM_Hardware_Table[PWM_DEVICE_CNT] = {
    //[PWM_WS2812]    = {&htim3,  TIM_CHANNEL_2, CHANNEL_TYPE_NORMAL},
    [PWM_BUZZER]    = {&htim12,  TIM_CHANNEL_2, CHANNEL_TYPE_NORMAL},
    [PWM_TEMP_CTRL] = {&htim8,  TIM_CHANNEL_3, CHANNEL_TYPE_COMP},
};

/**
 * @brief 集中启动所有在表中注册的 PWM 硬件通道
 */
void TIM_PWM_Init(void)
{
    for (int i = 0; i < PWM_DEVICE_CNT; i++) {
        // 安全防御：跳过未分配硬件的虚职槽位
        if (PWM_Hardware_Table[i].htim == NULL) continue;
        // 识别通道属性，分流调用 HAL 核心库函数
        if (PWM_Hardware_Table[i].type == CHANNEL_TYPE_COMP) {
            // 启动高级定时器特有的互补通道
            HAL_TIMEx_PWMN_Start(PWM_Hardware_Table[i].htim, PWM_Hardware_Table[i].channel);
        } else {
            // 启动通用定时器的普通正向通道
            HAL_TIM_PWM_Start(PWM_Hardware_Table[i].htim, PWM_Hardware_Table[i].channel);
        }
    }
}

/**
 * @brief 改变指定设备的占空比 (CCR)
 */
void TIM_Set_Compare(PWM_Device_e device, uint32_t compare)
{
    if (device >= PWM_DEVICE_CNT) return;
    if (PWM_Hardware_Table[device].htim == NULL) return;

    __HAL_TIM_SET_COMPARE(PWM_Hardware_Table[device].htim,
                          PWM_Hardware_Table[device].channel,
                          compare);
}
/**
 * @brief 改变指定定时器的周期/频率 (ARR)
 * @note 蜂鸣器唱歌、输出特定频率脉冲时使用
 */
void TIM_Set_Autoreload(PWM_Device_e device, uint32_t autoreload)
{
    if (device >= PWM_DEVICE_CNT) return;
    if (PWM_Hardware_Table[device].htim == NULL) return;

    __HAL_TIM_SET_AUTORELOAD(PWM_Hardware_Table[device].htim, autoreload);
}

/**
 * @brief 强符号重写：HAL 库脉冲半传输完成全局中断入口
 */
void HAL_TIM_PWM_PulseFinishedHalfCpltCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM3)
    {
        WS2812_DMA_Handler(1);
    }
}

/**
 * @brief 强符号重写：HAL 库脉冲全传输完成全局中断入口
 */
void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM3)
    {
        WS2812_DMA_Handler(0);
    }
}