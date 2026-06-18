//
// Created by qza on 2026/6/16.
//

#ifndef H7_FRAMEWORK_OFFLINE_DETECTOR_H
#define H7_FRAMEWORK_OFFLINE_DETECTOR_H

#include "stdint.h"
#include "stdbool.h"

typedef struct {
    //uint8_t dev_id;
   // void *dev_id;
    uint32_t last_feed_tick;    // 最后一次喂狗时间
    uint32_t last_check_tick;   // 最后一次检查这个外设的时间（新增！）
    uint8_t fail_count;           // 连续失败计数
    bool is_online;               // 当前在线状态
} Offline_Check_t;

typedef struct {
    uint32_t id;
    Offline_Check_t *device_ptr;
    uint32_t period_ms;         // 检测周期
    uint32_t timeout_ms;        // 超时阈值
    uint8_t max_fails;            // 允许连续失败上限（如设为3）
} Offline_Detector_t;

typedef struct {
    Offline_Check_t DJI_6020_Pitch;
    Offline_Check_t DJI_6020_Yaw;

    Offline_Check_t DJI_3508_Shoot_L;
    Offline_Check_t DJI_3508_Shoot_R;
    Offline_Check_t DJI_3508_Shoot_M;

    Offline_Check_t DJI_3508_Chassis[4];
    Offline_Check_t DJI_6020_Steer[4];

    Offline_Check_t DJI_3508_Pull;
    Offline_Check_t DJI_3508_Travel;
    Offline_Check_t DJI_3508_Yaw;
    Offline_Check_t DJI_2006_bo;

    Offline_Check_t DM4310_Feed;
    Offline_Check_t DM4310_Pitch;
    Offline_Check_t DM4310_Yaw;
    Offline_Check_t LK9025_Yaw;

    Offline_Check_t cap;

}All_Offline_Detector_t;

extern const Offline_Detector_t Offline_Detector_Config_Table[];

void Offline_Detect_Feed(uint32_t *Feed_time);
void Offline_Monitor(void);

#endif //H7_FRAMEWORK_OFFLINE_DETECTOR_H
