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

/**
 * 接受新的 TCP 连接并开始为其提供服务
 *
 * 该函数从指定的监听套接字中接受一个传入的 TCP 连接请求。
 * 调用 `accept` 函数处理传入连接，并返回一个新的套接字文件描述符，
 * 该文件描述符将用于与客户端通信。如果 `accept` 调用失败，则返回 -1。
 *
 * param listen_socket 监听套接字的文件描述符，必须已经调用 `bind` 和 `listen` 进行绑定和监听。
 * 
 * return 
 *   - 成功时，返回新连接的套接字文件描述符，用于后续的读写操作。
 *   - 失败时，返回 -1，表示 `accept` 调用出错。
 */
int CQuery_accept_tcp_connect(int listen_socket)
{
    // 定义一个 CQuery 指针，用于存储当前请求连接
    CQuery *query = NULL;
    if (NULL == (query = get_free_query()))
    {
        return RESOURCE_UNAVAILABLE;
    }

    // 定义地址结构体，用于存储客户端的地址信息
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);
    struct epoll_event ev;

    // 尝试接受来自客户端的新连接，保存到query->m_socket_fd中
    // 如果失败，检查错误类型：资源临时不可用（EAGAIN或EWOULDBLOCK）或其他错误
    if (0 > (query->m_socket_fd = accept(listen_socket, (struct sockaddr *)&addr, &addrlen)))
    {
        // 如果是EAGAIN或EWOULDBLOCK错误，表明资源暂时不可用，将query返回到空闲列表并返回错误码
        if (EAGAIN == errno || EWOULDBLOCK == errno)
        {
            add_free_list(query);
            return RESOURCE_TEMPOREARILY_UNAVAILABLE;
        }
        else
        {
            // 如果是其他错误，关闭连接，返回到空闲列表，并返回套接字接受错误
            CQuery_close_socket(query);
            add_free_list(query);
            return SOCKET_ACCEPT_ERROR;
        }
    }

    // 配置接受到的socket文件描述符，如果失败，关闭连接并返回错误码
    if (0 > config_socket(query->m_socket_fd))
    {
        CQuery_close_socket(query);
        add_free_list(query);
        return SOCKET_CONFIGUE_ERROR;
    }

    // 生成一个UUID，并将其保存到请求的缓冲区中
    char uuid[PLAYER_ID_LEN + 1];
    generate_uuid(uuid);
    CQuery_set_query_buffer(query, uuid, PLAYER_ID_LEN + 1);
    query->m_header.type = RESPONSE_UUID;   // 设置响应头类型为UUID

    // 输出服务器日志，表示接受到一个新连接并打印客户端地址信息
    printf("\033[32m%s\033[0m %s", "(server)", "accept new connection from: ");
    print_addr_info(&addr);
    printCurrentTime();

    // 输出服务器日志，表示尝试向客户端发送UUID并等待响应
    printf("\033[34m(server)\033[0m trying send uuid: \033[0;33m%s\033[0m to client, waiting for response...", uuid);
    printCurrentTime();

    // 设置用于接收数据的epoll事件，监听文件描述符的可读事件、边缘触发、错误、挂起等事件
    struct epoll_event new_evt;
    new_evt.events = EPOLLIN | EPOLLET | EPOLLERR | EPOLLHUP | EPOLLPRI;
    new_evt.data.fd = query->m_socket_fd;
    // 将新接收到的连接注册到全局的recv_epoll队列中，进行事件监听
    if (0 > epoll_ctl(g_recv_epoll_fd, EPOLL_CTL_ADD, CQuery_get_socket(query), &new_evt))
    { /*注册在recv_epoll的监听队列上*/
        // 如果注册失败，关闭连接并返回到空闲列表，返回epoll错误
        CQuery_close_socket(query);
        add_free_list(query);

        return EPOLL_ERROR;
    }
    
    // 将query添加到处理队列中，等待后续操作
    add_gready_list(query);
    return NO_ERROR;    // 一切正常，返回无错误
}

/**
 * 函数名称: CQuery_send_query
 * 功能: 将CQuery 之中携带的数据通过套接字发送到服务器
 * 参数:
 *   - query: 指向 `CQuery` 结构体的指针，该结构体包含了查询数据和套接字信息
 * 返回值:
 *   - 成功时返回已发送的字节数
 *   - 失败时返回 -2，表示套接字不可用或发送过程出错
 */
int CQuery_send_query(CQuery *query)
{
    // 首先检查套接字是否可用，如果不可用，则返回 -2 表示失败
    if (!CQuery_is_sock_ok(query))
        return -2;

    int send_byte = 0, have_send = 0;   // 临时变量，分别记录每次发送的字节数和已经发送的总字节数
    // 用一个while循环不断的写入数据，
    // 但是循环过程中的buf参数和nbytes参数是我们自己来更新的。
    // 返回值大于0，表示写了部分数据或者是全部的数据。
    // 返回值小于0，此时出错了，需要根据错误类型进行相应的处理*/
    while (have_send < query->m_query_len)
    { 
        /*
         * 调用 `write` 函数向套接字发送数据
         * query->m_socket_fd: 套接字文件描述符
         * query->m_byte_Query + have_send: 指向需要发送的数据的当前位置
         * query->m_query_len - have_send: 还需要发送的数据长度
         * 返回值 `send_byte` 表示实际发送的字节数
         */
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
        // 成功发送了部分数据，更新已经发送的字节数
        have_send += send_byte;
    }

    // 全部数据发送完毕，返回发送的总字节数
    return have_send;
}

/**
 * 函数名称: CQuery_recv_message
 * 功能: 接收来自客户端的消息并处理 TCP 连接状态和消息缓冲
 * 参数:
 *   - socketfd: 代表客户端的套接字文件描述符，用于接收和处理消息
 * 返回值:
 *   - 成功时返回 NO_ERROR（定义的常量），表示消息成功处理
 *   - 失败时返回相应的错误代码，如 `-1` 表示套接字关闭或其他读取错误
 */
int CQuery_recv_message(int socketfd)
{
    // 定义一个结构体 `tcp_info`，用于获取 TCP 连接的状态信息
    struct tcp_info tcpinfo;
    socklen_t len = sizeof(tcpinfo);

    // 使用 `getsockopt` 获取套接字的 TCP 连接状态
    if (getsockopt(socketfd, IPPROTO_TCP, TCP_INFO, &tcpinfo, &len) == -1)
    {
        // 如果获取失败，打印错误信息并返回 -1
        perror("getsockopt failed");
        return -1;
    }

    // 检查 TCP 状态是否为 `TCP_CLOSE` 或 `TCP_CLOSE_WAIT`
    // 表示客户端已经断开连接或即将断开连接
    if (tcpinfo.tcpi_state == TCP_CLOSE || tcpinfo.tcpi_state == TCP_CLOSE_WAIT)
    {
        // 从 epoll 中删除该套接字的监听
        epoll_ctl(g_recv_epoll_fd, EPOLL_CTL_DEL, socketfd, NULL);
        epoll_ctl(g_send_epoll_fd, EPOLL_CTL_DEL, socketfd, NULL);

        // 根据套接字文件描述符获取对应的玩家信息
        player_info *info = get_player_info_by_sock(socketfd);
        if (info)
        {
            // 打印客户端退出信息
            printf("\033[31m");
            printf("(server)client quit: ");
            printf("\033[0m");
            printCurrentTime();
            printf("\033[31m");
            printf("(server)client id: %s, name: %s, socketfd: %d\n", info->id, info->name, info->socketfd);
            printf("\033[0m");

            // 将玩家标记为不可用
            info->available = false;

            CQuery *query;
            // 通知其他玩家有人退出
            while (NULL == (query = get_free_query()))
                ;   // 获取空闲的数据请求对象
            query->m_socket_fd = socketfd;
            query->m_header.type = SOME_ONE_QUIT;   // 设置消息类型为退出消息
            CQuery_set_query_buffer(query, info->id, PLAYER_ID_LEN); // 设置消息缓冲区内容为玩家 UUID
            add_gready_list(query); // 将消息加入等待处理队列
        }

        // close(socketfd);
        return -1;
    }

    int read_byte;  // 读取的字节数
    player_info *info = get_player_info_by_sock(socketfd);  // 获取玩家信息

    // 使用 `read` 函数从套接字读取数据，注意：这里是读到对应玩家的缓冲区之中
    while (0 < (read_byte = read(socketfd, info->rcv_buffer + info->havent_handle, UNIT_BUFFER_SIZE)))
    {
        // 更新未处理的数据长度
        info->havent_handle += read_byte;
        // printf("read_byte: %d, havent_hanlde: %d\n", read_byte, havent_handle);
    }

    // 检查 `errno` 是否为 `EAGAIN` 或 `EWOULDBLOCK`
    // 这表示读取的数据不足或阻塞
    if (EAGAIN == errno || EWOULDBLOCK == errno)
    {
        int have_handle;    // 处理的数据长度
        bool is_body_handled = false;
        // 循环处理接收到的消息
        // info->prepare_to_handle 表示需要处理的数据长度。
        // info->havent_handle 表示当前接收缓冲区中尚未处理的字节数。
        // 该 while 循环的目的是确保在缓冲区中有足够的数据可以处理时（prepare_to_handle 小于或等于 havent_handle），继续处理。
        while (info->prepare_to_handle <= info->havent_handle)
        {
            // info->is_header_handled == true 说明消息的头部已经被正确解析，现在需要处理消息体。
            if (info->is_header_handled)
            {
                // memmove 函数将接收缓冲区中的数据（消息体）移动到 query 对象的 m_byte_Query 字段中。
                memmove(info->query->m_byte_Query, info->rcv_buffer, info->query->m_header.length);
                // 然后，将消息体的长度设置为 query->m_query_len，并将其添加到 gready_list 列表中，供后续处理。
                info->query->m_query_len = info->query->m_header.length;
                add_gready_list(info->query);

                // 处理完消息体后，将 is_header_handled 标志设置为 false，
                // 并更新 have_handle 和 prepare_to_handle，准备处理下一个消息头。
                info->is_header_handled = false;
                have_handle = info->query->m_header.length;
                info->prepare_to_handle = header_size;
            }
            else
            {
                // 如果 is_header_handled == false，表示当前还没有处理消息头，因此首先需要获取一个新的 CQuery 对象。
                if (NULL == (info->query = get_free_query()))
                    return RESOURCE_UNAVAILABLE;
                // 之后，将当前套接字描述符赋值给 query->m_socket_fd，
                // 并使用 memmove 将接收缓冲区中的数据（消息头部）拷贝到 query->m_header。
                info->query->m_socket_fd = socketfd;
                memmove(&info->query->m_header, info->rcv_buffer, header_size);
                // printf("message type: %d, message length: %d\n", header->type, header->length);
 
                // 处理完消息头部后，设置 is_header_handled = true，
                // 并更新 have_handle 和 prepare_to_handle，以准备处理消息体。
                info->is_header_handled = true;
                have_handle = header_size;
                info->prepare_to_handle = info->query->m_header.length;
            }

            // 通过 memmove，将未处理的字节向缓冲区的前端移动，确保缓冲区紧凑，方便后续继续接收数据。
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


/*
 * 函数名称: CQuery_check_socket_err
 * 功能: 检查套接字是否存在错误并返回相应的错误码
 * 参数:
 *   - query: 指向 `CQuery` 结构体的指针，该结构体包含了与套接字相关的信息
 * 返回值:
 *   - 成功时返回套接字的错误码
 *   - 失败时返回 -1，表示 `getsockopt` 调用出错
 */
int CQuery_check_socket_err(const CQuery *query)
{
    int error; // 用来存储从套接字获取的错误码
    int len = sizeof(error); // `getsockopt` 要求的参数，表示错误码的大小

    // 调用 `getsockopt` 获取套接字上最后发生的错误
    // query->m_socket_fd: 套接字文件描述符
    // SOL_SOCKET: 指定套接字层次（要检查的是套接字级别的选项）
    // SO_ERROR: 获取套接字的错误状态
    // &error: 输出参数，保存返回的错误码
    // (socklen_t *)&len: 输入输出参数，表示错误码的大小
    if (getsockopt(query->m_socket_fd, SOL_SOCKET, SO_ERROR, &error, (socklen_t *)&len) < 0)
    {
        // 如果 `getsockopt` 调用失败，返回 -1 表示出错
        return -1;
    }
    else
        // 如果 `getsockopt` 成功，返回获取到的错误码
        return error;
}

void CQuery_pack_message(CQuery *query)
{
    pack_message(query->m_header.type, query->m_byte_Query, &query->m_query_len, query->m_byte_Query);
}

bool CQuery_is_sock_ok(const CQuery *query)
{
    return (query->m_socket_fd > 0);
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
