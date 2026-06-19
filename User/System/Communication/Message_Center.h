//
// Created by CaoKangqi on 2026/6/19.
//

#ifndef H7_FRAMEWORK_MESSAGE_CENTER_H
#define H7_FRAMEWORK_MESSAGE_CENTER_H

#include <stdint.h>
#include <stddef.h>

typedef struct {
    char name[16];      // 消息名称
    void *data_ptr;     // 指向数据的内存地址
    size_t size;        // 数据大小
} Topic_t;

// 注册频道，由开发者指定数据存放地址
void PubRegister(const char *name, void *data, size_t size);
void SubRegister(const char *name, void *buffer, size_t size);

// 推送/获取消息
void PubPushMessage(const char *name, void *data);
void SubGetMessage(const char *name, void *buffer);

#endif //H7_FRAMEWORK_MESSAGE_CENTER_H
