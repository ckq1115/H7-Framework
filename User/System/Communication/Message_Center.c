//
// Created by CaoKangqi on 2026/6/19.
//

#include "Message_Center.h"
#include <string.h>
#include "cmsis_os2.h"

#include "DBUS.h"
#include "All_Motor.h"
#include "IMU_Task.h"

#define MAX_TOPICS      32
#define MAX_NAME_LEN    32

// 内部主题结构体
typedef struct {
    char name[MAX_NAME_LEN]; // 频道名称
    void *data_ptr;          // 指向真实的外部全局变量内存 (核心变更)
    size_t size;             // 结构体大小
    uint8_t is_used;         // 是否被占用
} Real_Topic_t;

static Real_Topic_t g_topics[MAX_TOPICS];
static uint8_t g_topic_count = 0;
static osMutexId_t g_center_mutex = NULL; // 仅用于保护主题注册列表本身的锁

/**
 * @brief 内部查找或创建频道函数
 */
static Real_Topic_t* Find_Or_Create_Topic(const char *name, size_t size) {
    uint8_t use_mutex = 0;
    // 判断当前是否需要加锁
    if (osKernelGetState() >= osKernelRunning) {
        if (g_center_mutex == NULL) {
            g_center_mutex = osMutexNew(NULL);
        }
        if (g_center_mutex != NULL) {
            use_mutex = 1; // 标记本次操作需要使用锁
            osMutexAcquire(g_center_mutex, osWaitForever);
        }
    }
    // 检查是否已经存在同名 Topic
    for (uint8_t i = 0; i < g_topic_count; i++) {
        if (strcmp(g_topics[i].name, name) == 0) {
            if (use_mutex) {
                osMutexRelease(g_center_mutex);
            }
            return &g_topics[i];
        }
    }
    // 不存在则新建一个
    if (g_topic_count >= MAX_TOPICS) {
        if (use_mutex) {
            osMutexRelease(g_center_mutex);
        }
        return NULL;
    }
    Real_Topic_t *t = &g_topics[g_topic_count];
    strncpy(t->name, name, MAX_NAME_LEN - 1); // 留一个字节给'\0'，防止越界
    t->name[MAX_NAME_LEN - 1] = '\0';
    t->size = size;
    t->data_ptr = NULL;
    t->is_used = 1;
    g_topic_count++;
    // 统一释放锁
    if (use_mutex) {
        osMutexRelease(g_center_mutex);
    }
    return t;
}

Publisher_t* PubRegister(const char *name, void *external_ptr, size_t size) {
    if (external_ptr == NULL) return NULL;
    Real_Topic_t *t = Find_Or_Create_Topic(name, size);
    if (t) {
        t->data_ptr = external_ptr; // 强行绑定硬件全局变量地址
    }
    return (Publisher_t*)t;
}

Subscriber_t* SubRegister(const char *name, size_t size) {
    // 订阅者如果先启动，先抢占格位，后续由 PubRegister 注入实体地址
    return (Subscriber_t*)Find_Or_Create_Topic(name, size);
}

void PubPushMessage(Publisher_t *pub_handle, const void *data) {
    Real_Topic_t *t = (Real_Topic_t*)pub_handle;
    if (t == NULL || data == NULL || t->data_ptr == NULL) return;

    uint32_t primask_bit = __get_PRIMASK();
    __disable_irq();

    memcpy(t->data_ptr, data, t->size);

    if (!primask_bit) {
        __enable_irq();
    }
}

void SubGetMessage(Subscriber_t *sub_handle, void *buffer) {
    Real_Topic_t *t = (Real_Topic_t*)sub_handle;
    if (t == NULL || buffer == NULL || t->data_ptr == NULL) return;

    uint32_t primask_bit = __get_PRIMASK();
    __disable_irq();

    memcpy(buffer, t->data_ptr, t->size);

    if (!primask_bit) {
        __enable_irq();
    }
}

void Message_Center_Init(void) {
    if (g_center_mutex == NULL) {
        g_center_mutex = osMutexNew(NULL);
    }

    PubRegister("dbus_data",  &DBUS,      sizeof(DBUS));
    PubRegister("imu_data",   &IMU_Data,  sizeof(IMU_Data));

    PubRegister("chassis_motors", &chassis_motors, sizeof(Chassis_Motor_Group_t));
    PubRegister("gimbal_motors",  &gimbal_motors,  sizeof(Gimbal_Motor_Group_t));
    PubRegister("shoot_motors",   &shoot_motors,   sizeof(Shoot_Motor_Group_t));
}