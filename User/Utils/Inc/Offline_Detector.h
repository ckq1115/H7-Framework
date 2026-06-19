//
// Created by qza on 2026/6/16.
//

#ifndef H7_FRAMEWORK_OFFLINE_DETECTOR_H
#define H7_FRAMEWORK_OFFLINE_DETECTOR_H

#include <stdint.h>
#include <stdbool.h>

// 分组枚举
typedef enum {
    GROUP_NONE = 0,
    CHASSIS,
    GIMBAL,
    SHOOT,
    GROUP_ALL
} Device_Group_e;

// 离线检测基础结构体 (必须被嵌套在各个传感器/电机的数据结构体内)
typedef struct {
    uint32_t last_feed_tick;
    bool is_online;
} Offline_Check_t;

// 离线监控表条目
typedef struct {
    Offline_Check_t *node;  // 明确指向具体的 offline 变量地址
    uint32_t timeout_ms;    // 超时阈值
    Device_Group_e group;   // 所属分组
} Offline_Route_t;

// 函数声明
void Offline_Monitor(void);
bool Is_Group_Online(Device_Group_e group);

#endif //H7_FRAMEWORK_OFFLINE_DETECTOR_H
