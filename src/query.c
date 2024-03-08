#include "query.h"
#include "query_list.h"

CQuery *CQuery_create()
{
    CQuery *query = (CQuery *)malloc(sizeof(CQuery));
    if (NULL == query)
        return NULL;

    CQuery_init(query);
    return query;
}

void CQuery_init(CQuery *query)
{
    query->m_header.type = UNKWON_TYPE;
    query->m_header.length = 0;
    query->m_socket_fd = -1;

    memset(query->m_byte_Query, 0, QUERY_BUFFER_LEN);
    query->m_query_len = -1;
    query->p_pre_query = NULL;
    query->p_next_query = NULL;
}

void CQuery_destroy(CQuery *query)
{
    if (query)
    {
        CQuery_close_socket(query);
        free(query);
    }
}

// 作为服务器时，接收请求的TCP连接，建立连接后保存到m_socket_fd待用
int CQuery_accept_tcp_connect(int listen_socket)
{
    CQuery *query = NULL;
    if (NULL == (query = get_free_query()))
    {
        return RESOURCE_UNAVAILABLE;
    }

    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);
    struct epoll_event ev;

    if (0 > (query->m_socket_fd = accept(listen_socket, (struct sockaddr *)&addr, &addrlen)))
    {
        if (EAGAIN == errno || EWOULDBLOCK == errno)
        {
            add_free_list(query);
            return RESOURCE_TEMPOREARILY_UNAVAILABLE;
        }
        else
        {
            CQuery_close_socket(query);
            add_free_list(query);
            return SOCKET_ACCEPT_ERROR;
        }
    }

    if (0 > config_socket(query->m_socket_fd))
    {
        CQuery_close_socket(query);
        add_free_list(query);
        return SOCKET_CONFIGUE_ERROR;
    }

    char uuid[PLAYER_ID_LEN + 1];
    generate_uuid(uuid);
    CQuery_set_query_buffer(query, uuid, PLAYER_ID_LEN + 1);
    query->m_header.type = RESPONSE_UUID;

    printf("\033[32m%s\033[0m %s", "(server)", "accept new connection from: ");
    print_addr_info(&addr);
    printCurrentTime();

    printf("\033[34m(server)\033[0m trying send uuid: \033[0;33m%s\033[0m to client, waiting for response...", uuid);
    printCurrentTime();

    struct epoll_event new_evt;
    new_evt.events = EPOLLIN | EPOLLET | EPOLLERR | EPOLLHUP | EPOLLPRI;
    new_evt.data.fd = query->m_socket_fd;
    if (0 > epoll_ctl(g_recv_epoll_fd, EPOLL_CTL_ADD, CQuery_get_socket(query), &new_evt))
    { /*注册在recv_epoll的监听队列上*/
        CQuery_close_socket(query);
        add_free_list(query);

        return EPOLL_ERROR;
    }

    add_gready_list(query);
    return NO_ERROR;
}

int CQuery_send_query(CQuery *query)
{
    if (!CQuery_is_sock_ok(query))
        return -2;

    int send_byte = 0, have_send = 0;
    while (have_send < query->m_query_len)
    { /*用一个while循环不断的写入数据，但是循环过程中的buf参数和nbytes参数是我们自己来更新的。返回值大于0，表示写了部分数据或者是全部的数据。返回值小于0，此时出错了，需要根据错误类型进行相应的处理*/
        send_byte = write(query->m_socket_fd,
                          query->m_byte_Query + have_send,
                          query->m_query_len - have_send); /*将socket当普通文件进行读写就可以*/
        if (send_byte <= 0)
        {
            if (errno == EINTR || EAGAIN == errno)
            { /*EINTR 此调用被信号所中断；EAGAIN 当使用不可阻断I/O 时（O_NONBLOCK），若无数据可读取则返回此值。*/
                continue;
            }
            else
            {
                CQuery_close_socket(query);
                return -2;
            }
        }
        have_send += send_byte;
    }

    return have_send;
}

int CQuery_recv_message(int socketfd)
{
    // 获取 TCP 连接信息
    struct tcp_info tcpinfo;
    socklen_t len = sizeof(tcpinfo);

    if (getsockopt(socketfd, IPPROTO_TCP, TCP_INFO, &tcpinfo, &len) == -1)
    {
        perror("getsockopt failed");
        return -1;
    }

    if (tcpinfo.tcpi_state == TCP_CLOSE || tcpinfo.tcpi_state == TCP_CLOSE_WAIT)
    {
        epoll_ctl(g_recv_epoll_fd, EPOLL_CTL_DEL, socketfd, NULL);
        epoll_ctl(g_send_epoll_fd, EPOLL_CTL_DEL, socketfd, NULL);

        player_info *info = get_player_info_by_sock(socketfd);
        if (info)
        {
            printf("\033[31m");
            printf("(server)client quit: ");
            printf("\033[0m");
            printCurrentTime();
            printf("\033[31m");
            printf("(server)client id: %s, name: %s, socketfd: %d\n", info->id, info->name, info->socketfd);
            printf("\033[0m");

            info->available = false;

            CQuery *query;
            // 通知其他玩家有人退出
            while (NULL == (query = get_free_query()))
                ;
            query->m_socket_fd = socketfd;
            query->m_header.type = SOME_ONE_QUIT;
            CQuery_set_query_buffer(query, info->id, PLAYER_ID_LEN);
            add_gready_list(query);
        }

        // close(socketfd);
        return -1;
    }

    int read_byte;
    player_info *info = get_player_info_by_sock(socketfd);

    while (0 < (read_byte = read(socketfd, info->rcv_buffer + info->havent_handle, UNIT_BUFFER_SIZE)))
    {
        info->havent_handle += read_byte;
        // printf("read_byte: %d, havent_hanlde: %d\n", read_byte, havent_handle);
    }

    if (EAGAIN == errno || EWOULDBLOCK == errno)
    {
        // printf("read error(eagain || ewouldblock): %d\n\n", errno);
        int have_handle;
        bool is_body_handled = false;
        while (info->prepare_to_handle <= info->havent_handle)
        {
            if (info->is_header_handled)
            {
                memmove(info->query->m_byte_Query, info->rcv_buffer, info->query->m_header.length);
                info->query->m_query_len = info->query->m_header.length;
                add_gready_list(info->query);

                info->is_header_handled = false;
                have_handle = info->query->m_header.length;
                info->prepare_to_handle = header_size;
            }
            else
            {
                if (NULL == (info->query = get_free_query()))
                    return RESOURCE_UNAVAILABLE;
                info->query->m_socket_fd = socketfd;
                memmove(&info->query->m_header, info->rcv_buffer, header_size);
                // printf("message type: %d, message length: %d\n", header->type, header->length);

                info->is_header_handled = true;
                have_handle = header_size;
                info->prepare_to_handle = info->query->m_header.length;
            }

            memmove(info->rcv_buffer, info->rcv_buffer + have_handle, info->havent_handle - have_handle);
            info->havent_handle -= have_handle;
            // printf("have_handle: %d, havent_handle: %d\n\n", have_handle, havent_handle);
        }
    }
    else if (-1 == read_byte)
    {
        // TODO 读取出错逻辑
        printf("read error: %d\n\n", errno);
        return SOCKET_ACCEPT_ERROR;
    }

    return NO_ERROR;
}

int CQuery_close_socket(CQuery *query)
{
    if (0 <= query->m_socket_fd)
    {
        while (close(query->m_socket_fd) && (EINTR == errno))
            ;
        query->m_socket_fd = -1;
    }

    return 0;
}

bool CQuery_is_sock_ok(const CQuery *query)
{
    return (query->m_socket_fd > 0);
}

int CQuery_check_socket_err(const CQuery *query)
{
    int error;
    int len = sizeof(error);

    if (getsockopt(query->m_socket_fd, SOL_SOCKET, SO_ERROR, &error, (socklen_t *)&len) < 0)
    {
        return -1;
    }
    else
        return error;
}

int CQuery_set_query_sock(CQuery *query, int sock)
{
    if (0 < query->m_socket_fd && 0 < sock)
    {
        CQuery_close_socket(query);
    }
    query->m_socket_fd = sock;
}

int CQuery_get_socket(CQuery *query)
{
    return query->m_socket_fd;
}

int CQuery_set_query_buffer(CQuery *query, const char *pBuf, int buf_len)
{
    memmove(query->m_byte_Query, pBuf, buf_len);
    query->m_query_len = buf_len;

    return 0;
}

char *CQuery_get_query_buffer(CQuery *query)
{
    return query->m_byte_Query;
}

int CQuery_get_query_len(CQuery *query)
{
    return query->m_query_len;
}

int CQuery_set_pre_query(CQuery *query, CQuery *pQuery)
{
    query->p_pre_query = pQuery;
    return 0;
}

struct _CQuery *CQuery_get_pre_query(CQuery *query)
{
    return query->p_pre_query;
}

int CQuery_set_next_query(CQuery *query, CQuery *pQuery)
{
    query->p_next_query = pQuery;
    return 0;
}

struct _CQuery *CQuery_get_next_query(CQuery *query)
{
    return query->p_next_query;
}

void CQuery_pack_message(CQuery *query)
{
    pack_message(query->m_header.type, query->m_byte_Query, &query->m_query_len, query->m_byte_Query);
}