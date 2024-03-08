#ifndef BINARY_PROTOCOL_H
#define BINARY_PROTOCOL_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
// #include "query.h"

// 定义消息类型
typedef enum
{
    UNKWON_TYPE = 1,    // 未知类型, 用于初始化
    RESPONSE_UUID,      // 响应UUID, 用于客户端初始化自己的UUID
    GLOBAL_PLAYER_INFO, // 全局玩家信息, 用于客户端初始化其他玩家的相关信息
    SOME_ONE_JOIN,      // 有玩家加入, 用于客户端初始化其他玩家的相关信息
    SOME_ONE_QUIT,      // 有玩家退出, 用于客户端初始化其他玩家的相关信息
    GAME_UPDATE,        // 游戏更新, 用于客户端更新游戏状态
    PLAYER_INFO_CERT,   // 玩家信息认证, 用于客户端向服务器端认证玩家信息
    CLIENT_READY        // 客户端准备就绪, 用于客户端通知服务器自己已经准备就绪
} MessageType;

// 定义消息头
typedef struct
{
    MessageType type; // 消息类型
    uint16_t length;  // 消息长度
} MessageHeader;

extern int header_size;

// 从字节流之中提取出消息头
#define extractHeader(header, buffer) memmove(header, buffer, header_size)

// 将消息打包成二进制数据
void pack_message(MessageType type, void *data, uint16_t *dataLength, char *buffer);

// 将二进制数据解包成消息
int unpack_message(MessageHeader *header, char *buffer, int len);
#endif