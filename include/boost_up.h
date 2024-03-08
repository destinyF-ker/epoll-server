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
    if (0 > (g_send_epoll_fd = epoll_create(g_query_num)))
        return -2;

    if (0 > (g_recv_epoll_fd = epoll_create(g_query_num)))
        return -2;

    struct epoll_event ep_evt;
    ep_evt.events = EPOLLIN | EPOLLET;
    ep_evt.data.fd = g_pconf->listen_socket;
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