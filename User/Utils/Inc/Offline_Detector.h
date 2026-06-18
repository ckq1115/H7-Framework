//
// Created by qza on 2026/6/16.
//

#ifndef H7_FRAMEWORK_OFFLINE_DETECTOR_H
#define H7_FRAMEWORK_OFFLINE_DETECTOR_H

#include "stdint.h"
#include "stdbool.h"

typedef enum {
    GROUP_NONE = 0,     // 无分组
    CHASSIS,      // 底盘组
    GIMBAL,       // 云台组
    SHOOT,        // 发射机构组
    GROUP_ALL           // 全部设备
} Device_Group_e;

typedef struct {
    uint32_t last_feed_tick;    // 最后一次喂狗时间
    bool is_online;               // 当前在线状态
} Offline_Check_t;

void Offline_Detect_Feed(uint32_t *Feed_time);
void Offline_Monitor(void);
bool Is_Group_Online(Device_Group_e group);

#endif //H7_FRAMEWORK_OFFLINE_DETECTOR_H
