#ifndef __RECV_RES_h__
#define __RECV_RES_h__

#include <stddef.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include "server_conf.h"
#include "query.h"
#include "query_list.h"
#include "handler.h"

typedef int (*CALL_BACK)(char *, int);

void *recv_res_main(void *);

extern size_t g_query_num;
extern int g_recv_epoll_fd;
extern pconf_t *g_pconf;
extern bool g_over;

#endif