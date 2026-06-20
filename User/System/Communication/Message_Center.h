//
// Created by CaoKangqi on 2026/6/19.
//

#ifndef H7_FRAMEWORK_MESSAGE_CENTER_H
#define H7_FRAMEWORK_MESSAGE_CENTER_H

#include <stdint.h>
#include <stddef.h>

// 句柄别名，对齐第一份代码的业务层调用
typedef void Publisher_t;
typedef void Subscriber_t;

/**
 * @brief 注册发布频道（主要供消息中心初始化或任务初始化时绑定全局结构体）
 * @param name 消息频道的唯一名字 (例如 "imu_data")
 * @param external_ptr 指向外部全局结构体的指针 (例如 &IMU_Data)
 * @param size 数据结构体的大小
 * @return 成功返回发布者句柄，失败返回 NULL
 */
Publisher_t* PubRegister(const char *name, void *external_ptr, size_t size);

/**
 * @brief 订阅消息频道
 * @param name 想要订阅的消息频道名字
 * @param size 预期的数据结构体大小
 * @return 成功返回订阅者句柄，失败返回 NULL
 */
Subscriber_t* SubRegister(const char *name, size_t size);

/**
 * @brief 推送/更新消息（任务中使用。注意：如果中断直接写了绑定内存，可不调用此函数）
 * @param pub_handle 发布者句柄
 * @param data 指向待推送的本地数据的指针
 */
void PubPushMessage(Publisher_t *pub_handle, const void *data);

/**
 * @brief 获取消息（应用层任务安全、高效率地获取最新数据）
 * @param sub_handle 订阅者句柄
 * @param buffer 本地接收变量的缓冲区指针
 */
void SubGetMessage(Subscriber_t *sub_handle, void *buffer);

/**
 * @brief 消息中心集中初始化
 */
void Message_Center_Init(void);

#endif //H7_FRAMEWORK_MESSAGE_CENTER_H