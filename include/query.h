#ifndef __QUERY_H__
#define __QUERY_H__

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <stdbool.h>

#include "server_conf.h"
#include "binary_protocol.h"
#include "util.h"
#include "config.h"

/*接收数据的缓冲大小*/
#define TIME_OUT 1000

typedef struct _CQuery
{
    MessageHeader m_header; // 消息头
    int m_socket_fd;        // socket fd

    char m_byte_Query[QUERY_BUFFER_LEN]; // 携带的数据
    uint16_t m_query_len;                //  query长度
    struct _CQuery *p_pre_query;         // 上一个req
    struct _CQuery *p_next_query;        // 下一个req
} CQuery;

extern int g_send_epoll_fd; /*发送epoll*/
extern int g_recv_epoll_fd; /*接收epoll*/

extern int header_size;

CQuery *CQuery_create();
void CQuery_init(CQuery *query);
void CQuery_destroy(CQuery *query);

int CQuery_accept_tcp_connect(int listen_socket);
int CQuery_send_query(CQuery *query);
int CQuery_recv_message(int socketfd);

int CQuery_close_socket(CQuery *query);

bool CQuery_is_sock_ok(const CQuery *query);

int CQuery_check_socket_err(const CQuery *query);

int CQuery_set_query_sock(CQuery *query, int sock);
int CQuery_get_socket(CQuery *query);

int CQuery_set_query_buffer(CQuery *query, const char *pBuf, int buf_len);
char *CQuery_get_query_buffer(CQuery *query);

int CQuery_get_query_len(CQuery *query);

int CQuery_set_pre_query(CQuery *query, CQuery *pQuery);
CQuery *CQuery_get_pre_query(CQuery *query);

int CQuery_set_next_query(CQuery *query, CQuery *pQuery);
CQuery *CQuery_get_next_query(CQuery *query);

void CQuery_pack_message(CQuery *query);
extern pconf_t *g_pconf;

#endif