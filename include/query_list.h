#ifndef __QUERY_LIST_H__
#define __QUERY_LIST_H__

#include <pthread.h>
#include <stdio.h>
#include "query.h"
#include "player_info_array.h"

int init_query_list_lock();
int destroy_query_list_lock();

CQuery *get_free_query();
int add_free_list(CQuery *pQuery);

CQuery *get_gready_query();
int add_gready_list(CQuery *pQuery);
CQuery *get_gwork_query();
int add_gwork_list(CQuery *pQuery);
int del_gwork_list(CQuery *pQuery);

extern CQuery *g_pfree_list;
extern CQuery *g_pready_list;
extern CQuery *g_pwork_list;
extern CQuery *g_pwork_list_tail;  /*发送或接收结果状态的CQuery队列尾*/
extern CQuery *g_pready_list_tail; /*有数据准备建立连接的CQuery队列尾*/

#endif