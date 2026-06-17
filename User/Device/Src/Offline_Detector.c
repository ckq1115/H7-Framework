//
// Created by qza on 2026/6/16.
//
#include "stm32h7xx_hal.h"

void Offline_Detect_Feed(uint32_t *Feed_time) {
    uint32_t now = HAL_GetTick();
    *Feed_time = now;
}

#include "All_Task.h"

const Offline_Detector_t Offline_Detector_Config_Table[] = {
/*can*/
    {0, &All_Offline_Detector.DJI_3508_Chassis[0], 10, 10,1},
    {1, &All_Offline_Detector.DJI_3508_Chassis[1], 10, 10,1},
    {2, &All_Offline_Detector.DJI_3508_Chassis[2], 10, 10,1},
    {3, &All_Offline_Detector.DJI_3508_Chassis[3], 10, 10,1},
    {4, &All_Offline_Detector.DJI_6020_Pitch, 10,      10,1},

    {5, &All_Offline_Detector.DM4310_Feed, 10,         10,1},
    {6, &All_Offline_Detector.DJI_3508_Pull, 10,       10,1},
    {7, &All_Offline_Detector.DJI_3508_Yaw, 10,        10,1},
    {8, &All_Offline_Detector.cap, 10,                 10,1},// 超级电容

    { 9, &All_Offline_Detector.DJI_6020_Steer[0], 10,   10,1},
    {10, &All_Offline_Detector.DJI_6020_Steer[1], 10,   10,1},
    {11, &All_Offline_Detector.DJI_6020_Steer[2], 10,   10,1},
    {12, &All_Offline_Detector.DJI_6020_Steer[3], 10,   10,1},
};

void Offline_Monitor(void) {
    uint32_t now = HAL_GetTick();

    size_t table_size = sizeof(Offline_Detector_Config_Table) / sizeof(Offline_Detector_t);
    for (size_t i = 0; i < table_size; i++) {
        if ((now - Offline_Detector_Config_Table[i].device_ptr->last_check_tick) >= Offline_Detector_Config_Table[i].period_ms) {
            Offline_Detector_Config_Table[i].device_ptr->last_check_tick = now; // 更新节拍
            // 现在执行实际的超时检查
            if ((now - Offline_Detector_Config_Table[i].device_ptr->last_feed_tick) > Offline_Detector_Config_Table[i].timeout_ms) {
                Offline_Detector_Config_Table[i].device_ptr->fail_count++;
            }
            else Offline_Detector_Config_Table[i].device_ptr->fail_count=0;
            if (Offline_Detector_Config_Table[i].device_ptr->fail_count <= Offline_Detector_Config_Table[i].max_fails) {
                Offline_Detector_Config_Table[i].device_ptr->is_online = true;
            }
            else Offline_Detector_Config_Table[i].device_ptr->is_online = false;// 触发离线
        }
        else Offline_Detector_Config_Table[i].device_ptr->is_online = true;
    }
}