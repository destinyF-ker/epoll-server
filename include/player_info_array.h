#ifndef __PLAYER_INFO_ARRAY_H__
#define __PLAYER_INFO_ARRAY_H__

#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include "query.h"

typedef struct info_table {
    char *id;                     // 玩家ID，指向存储玩家唯一标识符的字符串。
    char *name;                   // 玩家名称，指向存储玩家昵称的字符串。
    int socketfd;                 // 与玩家对应的套接字文件描述符，用于网络通信。
    
    char rcv_buffer[UNIT_BUFFER_SIZE * 10]; // 接收缓冲区，存储从网络读取的未处理数据。
    CQuery *query;                // 指向当前与该玩家相关联的查询对象，用于处理玩家的请求。
    bool is_header_handled;       // 指示当前消息的头部是否已处理（`true` 表示已处理）。
    int prepare_to_handle;        // 指示准备处理的字节数，用于确定下一步应处理多少数据。
    int havent_handle;            // 接收缓冲区中未处理的字节数，表示从接收到的数据中还有多少需要处理。

    char snd_buffer[UNIT_BUFFER_SIZE * 10]; // 发送缓冲区，存储即将发送的数据。
    int havent_send;              // 发送缓冲区中尚未发送的字节数，用于追踪部分发送的消息。

    bool available;               // 标志玩家是否在线可用（`true` 表示在线，`false` 表示离线）。
    bool ready;                   // 标志玩家是否已准备好（可能与游戏逻辑相关，`true` 表示准备好）。
    int message_count;            // 玩家收到的消息计数，记录接收到的消息数量。
    pthread_mutex_t *msg_count_mutex; // 消息计数的互斥锁，确保多线程环境下对 `message_count` 的安全更新。

    struct info_table *next;      // 指向下一个玩家的指针，形成一个链表结构。
} player_info;

typedef struct {
    size_t length;               // 玩家信息表的长度，存储当前玩家数。
    player_info *head;           // 指向玩家链表的头节点。
} player_info_array;

extern player_info_array player_infos;

void init_player_info_array_lock();

void player_info_array_init();

int get_player_info_array_length();

int add_player_info(const char *id, int socketfd);

int set_player_name(char *id, char *name);

int delete_player_info(const char *id);

int delete_player_info_by_socketfd(int socketfd);

int clear_player_info_array();

player_info *get_player_info_by_sock(int socketfd);

int *get_player_info_sockfds();

char *package_global_player_info(int except_socketfd);

int scan_and_delete_unavailable_player_info();

void plus_message_count(player_info *info);

void minus_message_count(player_info *info);

int get_message_count(player_info *info);
#endif