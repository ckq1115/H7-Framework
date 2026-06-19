//
// Created by CaoKangqi on 2026/6/19.
//
#include "Message_Center.h"
#include <string.h>
#include "cmsis_os2.h"

#define MAX_TOPICS 32
static Topic_t g_topics[MAX_TOPICS];
static uint8_t g_topic_count = 0;

// 查找频道
static Topic_t* Find_Topic(const char *name) {
    for (uint8_t i = 0; i < g_topic_count; i++) {
        if (strcmp(g_topics[i].name, name) == 0) return &g_topics[i];
    }
    return NULL;
}

// 注册（发布者调用）
void PubRegister(const char *name, void *data, size_t size) {
    if (g_topic_count >= MAX_TOPICS) return;
    strcpy(g_topics[g_topic_count].name, name);
    g_topics[g_topic_count].data_ptr = data;
    g_topics[g_topic_count].size = size;
    g_topic_count++;
}

// 获取（订阅者调用）
void SubGetMessage(const char *name, void *buffer) {
    Topic_t *t = Find_Topic(name);
    if (t && buffer) {
        // 直接从源地址拷贝到订阅者缓冲区
        memcpy(buffer, t->data_ptr, t->size);
    }
}

// 推送（发布者调用）
void PubPushMessage(const char *name, void *data) {
    Topic_t *t = Find_Topic(name);
    if (t && data) {
        // 直接更新源地址数据
        memcpy(t->data_ptr, data, t->size);
    }
}