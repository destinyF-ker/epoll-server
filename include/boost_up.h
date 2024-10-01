#ifndef BOOST_UP_H
#define BOOST_UP_H

#include <stdlib.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <pthread.h>
#include <signal.h>
#include "server_conf.h"
#include "query.h"
#include "query_list.h"
#include "handler.h"
#include "send_req.h"
#include "recv_res.h"
#include "handler.h"
#include "player_info_array.h"

// 全局变量定义
player_info_array player_infos;    /*玩家信息链表*/
pconf_t *g_pconf = NULL;           /*服务器配置*/
CQuery *g_pfree_list = NULL;       /*无数据的CQuery队列*/
CQuery *g_pready_list = NULL;      /*有数据准备建立连接的CQuery队列*/
CQuery *g_pwork_list = NULL;       /*发送或接收结果状态的CQuery队列*/
CQuery *g_pwork_list_tail = NULL;  /*发送或接收结果状态的CQuery队列尾*/
CQuery *g_pready_list_tail = NULL; /*有数据准备建立连接的CQuery队列尾*/
int g_send_epoll_fd;               /*发送epoll*/
int g_recv_epoll_fd;               /*接收epoll*/
bool g_over = false;
bool g_is_write_eagain = false;
size_t g_query_num = MAX_QUERY_NUM;
int header_size = sizeof(MessageHeader);

int init_main()
{
    /*初始化CQuery队列*/
    g_pfree_list = (CQuery *)malloc(sizeof(struct _CQuery) * g_query_num);
    if (NULL == g_pfree_list)
        return -1;

    for (size_t i = 0; i < g_query_num; i++)
        CQuery_init(&g_pfree_list[i]);

    CQuery_set_pre_query(&g_pfree_list[0], NULL);
    CQuery_set_next_query(&g_pfree_list[0], &g_pfree_list[1]);
    for (size_t i = 1; i < g_query_num - 1; i++)
    {
        CQuery_set_pre_query(&g_pfree_list[i], &g_pfree_list[i - 1]);
        CQuery_set_next_query(&g_pfree_list[i], &g_pfree_list[i + 1]);
    }
    CQuery_set_pre_query(&g_pfree_list[g_query_num - 1], &g_pfree_list[g_query_num - 2]);
    CQuery_set_next_query(&g_pfree_list[g_query_num - 1], NULL);

    /* init epoll list */
    // epoll_create 是 Linux 提供的一个系统调用，专门用于创建一个 epoll 实例。
    // epoll 是一种高效的 I/O 事件通知机制，用于处理大量文件描述符的 I/O 事件，如网络连接、文件等。
    // 参数 size 表示初始监听的文件描述符的数量，虽然现代系统会忽略这个值，但它曾经用于分配内部数据结构的大小。
    if (0 > (g_send_epoll_fd = epoll_create(g_query_num)))
        return -2;

    if (0 > (g_recv_epoll_fd = epoll_create(g_query_num)))
        return -2;

    // epoll_event 之中包含有两个主要字段：
    // 1. events：表示感兴趣的时间类型
    // 2. data：通常用于存储用户数据，比如文件描述符（fd）
    struct epoll_event ep_evt;

    // EPOLLIN：表示监听文件描述符是否有可读事件（也就是数据可以从文件描述符读取，比如新的网络链接或者接收到的数据）
    // EPOLLET：表示启用边缘触发（Edge Triggered，ET）模式
    ep_evt.events = EPOLLIN | EPOLLET;

    // 存储与事件相关的文件描述符，在这里意味着监听这个套接字的读事件，并在事件发生时能够识别出是哪个文件描述符触发的。
    ep_evt.data.fd = g_pconf->listen_socket;

    // epoll_ctl 是 epoll 用于控制文件描述符和 epoll 实例之间关系的系统调用，它有三个主要操作：
    // EPOLL_CTL_ADD: 将指定的文件描述符添加到 epoll 实例中。
    // EPOLL_CTL_MOD: 修改已经在 epoll 实例中的文件描述符。
    // EPOLL_CTL_DEL: 从 epoll 实例中删除文件描述符。
    // 
    // 这个调用的含义是，将监听套接字（g_pconf->listen_socket）注册到 g_recv_epoll_fd 这个 epoll 实例中，
    // 并监听可读事件（EPOLLIN），同时启用边缘触发（EPOLLET）。
    epoll_ctl(g_recv_epoll_fd, EPOLL_CTL_ADD, g_pconf->listen_socket, &ep_evt);

    player_info_array_init();
    /*初始化共享锁*/
    init_player_info_array_lock();
    init_query_list_lock();

    return 0;
}

int clean_main()
{
    CQuery *tp = g_pwork_list;
    CQuery *tdel = NULL;
    while (NULL != tp)
    {
        CQuery_close_socket(tp);
        tdel = tp;
        tp = CQuery_get_next_query(tp);
        CQuery_destroy(tdel);
    }
    tp = g_pready_list;
    while (NULL != tp)
    {
        CQuery_close_socket(tp);
        tdel = tp;
        tp = CQuery_get_next_query(tp);
        CQuery_destroy(tdel);
    }
    tp = g_pfree_list;
    while (NULL != tp)
    {
        CQuery_close_socket(tp);
        tdel = tp;
        tp = CQuery_get_next_query(tp);
        CQuery_destroy(tdel);
    }

    /*关闭epoll socket*/
    close(g_send_epoll_fd);
    close(g_recv_epoll_fd);

    /*销毁共享锁*/
    destroy_query_list_lock();

    return 0;
}

#endif