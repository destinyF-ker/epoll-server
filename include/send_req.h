#ifndef __SEND_REQ_H__
#define __SEND_REQ_H__

#include <stddef.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include "query.h"
#include "query_list.h"

static int not_right;

void *send_req_main(void *);
void handle_group_send(CQuery *query);
void handle_send(int target_sock, CQuery *query);

extern int g_send_epoll_fd;
extern int g_recv_epoll_fd;
extern bool g_over;
extern bool g_is_write_eagain;
extern size_t g_query_num;

#endif