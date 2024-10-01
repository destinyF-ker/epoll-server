#include "send_req.h"

/**
 * @brief 游戏服务器中用于处理发送请求的主函数。
 * 
 * 该函数持续处理从服务器向客户端发送的出站数据，使用 `epoll_wait` 机制监控处于非阻塞状态的客户端的套接字事件
 * （尤其是之前在写操作中遇到 EAGAIN 错误的客户端）。
 * 
 * 函数的主要职责包括：
 * - 处理客户端的部分发送数据并重试写操作。
 * - 管理每个客户端的 `player_info` 数据，更新它们的发送缓冲区，并在数据完全发送后将它们从 epoll 监控中移除。
 * - 处理游戏更新消息、玩家加入/退出通知及其他与游戏相关的事件，并将其分发给相关客户端。
 * 
 * 该函数采用循环结构，持续运行直到全局标志 `g_over` 被设置为真，表示服务器应关闭。它还监控是否有玩家处于写阻塞状态
 * （通过 `g_is_write_eagain` 标志），并使用 `epoll` 机制以非阻塞方式处理客户端请求。
 * 
 * 在套接字通信期间处理错误，对于 `EAGAIN` 和 `EWOULDBLOCK` 情况会重试，而其他错误则使用 `perror` 进行记录。
 * 在遇到致命的套接字错误时，应实现将玩家踢出游戏的逻辑。
 * 
 * @param 无（该函数接收一个 `void` 指针作为参数，但在当前实现中未使用该参数）
 * 
 * @return `void*` （该函数返回一个 `void` 指针，但当前实现中从未使用该返回值）
 */
void *send_req_main(void *)
{
    not_right = 0;  // 初始化未完成发送的数据计数
    int tmp_count = 1;  // 用于调试，记录发送请求计数
    CQuery *pQuery = NULL;  // 当前处理的任务请求指针
    struct epoll_event ep_evt[MAX_EPOLL_EVENT]; // 存储 epoll 事件的数组 

    while (!g_over)
    {
        // printf("\033[32m%s\033[0m, not_right: %d\n", "send_req_main: ", not_right);
        // 如果有没有完成的发送请求 
        if (g_is_write_eagain)
        {   
            // 等待 epoll事件，超时设置为0表示非阻塞 
            int ready_num = epoll_wait(g_send_epoll_fd, ep_evt, MAX_EPOLL_EVENT, 0); /*等待事件*/

            // 处理每一个就绪的套接字
            for (int i = 0; i < ready_num; i++)
            {
                int sockfd = ep_evt[i].data.fd; // 获取就绪的套接字
                player_info *player = get_player_info_by_sock(sockfd);  // 获取对应的 player_info
                if (player == NULL)
                {
                    perror("get_player_info_by_sock");
                    continue;
                }
                if (!player->available) // 假如玩家已经是“等待被删除”状态，则直接跳过 
                    continue;
                // ========= ==发送数据==
                int have_write = write(sockfd, player->snd_buffer, player->havent_send);
                if (have_write > 0)
                {
                    player->havent_send -= have_write;  //更新没有发送数据的大小 
                    if (player->havent_send > 0)
                    {
                        // 将还没有发送完的数据移动到对应player_info的send_buffer中
                        memmove(player->snd_buffer, player->snd_buffer + have_write, player->havent_send);
                    }
                    else
                    {
                        // 将该socket从epoll中移除
                        epoll_ctl(g_send_epoll_fd, EPOLL_CTL_DEL, sockfd, NULL);
                        not_right--;    // 减少未完成发送的计数 
                    }
                }
                else
                {
                    if (errno == EAGAIN || errno == EWOULDBLOCK)
                        continue;
                    else
                    {
                        // TODO：这里应该将玩家踢出游戏，但是现在还没有实现
                        perror("write");
                        continue;
                    }
                }
            }

            // 如果没有未完成的发送任务，将 `g_is_write_eagain` 置为 false
            if (not_right == 0)
                g_is_write_eagain = false;
        }
        // 从待发送链表之中取一个待发送的数据包 
        if (NULL == (pQuery = get_gwork_query()))
            continue;

        // 获取该数据包对应的玩家信息 
        player_info *player = get_player_info_by_sock(pQuery->m_socket_fd);
        if (player == NULL)
        {
            perror("get_player_info_by_sock");
            continue;
        }
        // 更新玩家消息计数 
        minus_message_count(player);
        if (!player->available) // 假如玩家现在已经退出游戏，则释放该数据包 
        {
            add_free_list(pQuery);
            continue;
        }

        // 根据数据包类型来决定是要群发还是单发 
        if (pQuery->m_header.type == GAME_UPDATE ||
            pQuery->m_header.type == SOME_ONE_JOIN ||
            pQuery->m_header.type == SOME_ONE_QUIT)
        {
            // 如果有多个玩家，就进行群发 
            if (get_player_info_array_length() > 1)
                handle_group_send(pQuery);
            else
            {
                printf("(debug) %s\n", "send_req_main: no player info.");
            }
        }
        else
        {
            // 否则就单发 
            printf("send_req_main: %d:%d\n", pQuery->m_header.type, tmp_count++);
            handle_send(pQuery->m_socket_fd, pQuery);
        }

        // 释放对应的数据包（已经发送完了）
        add_free_list(pQuery);
    }
}

/**
 * @brief 处理群发消息的函数。
 * 
 * 该函数用于将来自某个客户端的消息发送给所有其他客户端（即群发）。消息由 `query` 参数中包含的套接字
 * 和相关数据提供，并被发送给所有连接的客户端，除去消息的发起者。
 * 
 * 函数的主要流程包括：
 * - 获取当前所有客户端的套接字列表，通过 `get_player_info_sockfds()` 函数获得。
 * - 检查套接字列表是否为空，或只有一个客户端连接的情况，如果满足条件则直接返回。
 * - 遍历套接字列表，将消息发送给每个除发起请求的客户端外的其他客户端。
 * - 对于每个客户端，调用 `handle_send()` 函数完成消息发送。
 * 
 * 在调试过程中，函数会打印各个阶段的调试信息，包括当前处理的套接字和其对应的状态。
 * 
 * @param query 指向包含要群发的消息信息的 `CQuery` 结构体的指针。该结构体中包含消息的发起者
 *              的套接字信息及消息内容。
 * 
 * @return 无返回值。
 */
void handle_group_send(CQuery *query)
{
    int sockfd = query->m_socket_fd;
    printf("(debug) %s%d\n", "handle_group_send(): ", sockfd);

    int *sockfds = get_player_info_sockfds();
    if (NULL == sockfds)
        return;
    printf("(debug) %s%d\n", "handle_group_send^^: ", sockfds[0]);
    if (1 == sockfds[0])
    {
        free(sockfds);
        return;
    }

    for (int i = 1; i <= sockfds[0]; i++)
    {
        printf("(debug) %s%d\n", "handle_group_send: ", sockfds[i]);
        if (sockfds[i] == sockfd)
            continue;
        printf("(debug) %s%d\n", "handle_group_send>>: ", sockfds[i]);
        handle_send(sockfds[i], query);
    }
}

/**
 * @brief 处理向指定客户端发送数据的函数。
 * 
 * 该函数将消息从服务器发送到指定客户端的套接字（`target_sock`）。消息数据由 `query` 参数提供，
 * 其中包含消息内容及其长度。根据发送的结果，函数可能会将未发送完的数据缓存在对应的 `player_info`
 * 结构体中，并利用 epoll 注册为可写事件以便稍后继续发送。
 * 
 * 函数的主要功能包括：
 * - 获取目标客户端的 `player_info` 结构体，并检查其可用性。
 * - 根据消息的类型（如玩家加入、退出、游戏更新等）输出调试信息。
 * - 如果目标客户端在上次发送操作中仍有未发送完的数据，则将新数据追加到该客户端的发送缓冲区中。
 * - 尝试立即向目标客户端发送数据。如果数据未完全发送（例如遇到 `EAGAIN` 错误），则将剩余数据
 *   缓存到 `player_info` 的发送缓冲区中，并在 epoll 中注册该套接字的写事件以便稍后继续发送。
 * - 处理发送过程中可能出现的错误（如 `EAGAIN` 或 `send` 失败）。
 * 
 * @param target_sock 目标客户端的套接字，表示消息要发送到的客户端。
 * @param query 指向包含待发送数据的 `CQuery` 结构体的指针，其中包含数据及其长度等信息。
 * 
 * @return 无返回值。
 */
void handle_send(int target_sock, CQuery *query)
{
    printf("\033[32m%s\033[0m\n", "handle_send");
    // 获取要发送的数据和数据长度 
    char *data = query->m_byte_Query;
    int data_size = query->m_query_len;
    int remain_size = data_size;    // 因为有可能会在中途对方的缓冲区已经收满
                                    // 所以要初始化剩余未发送的数据大小 

    // 获取目标客户端的 player_info 结构体
    player_info *player = get_player_info_by_sock(target_sock);
    if (player == NULL)
    {
        perror("get_player_info_by_sock");
        return;
    }

    // 如果玩家不可用（等待资源被清理完之后进行销毁），直接返回
    if (!player->available)
        return;

    // if (query->m_header.type == GLOBAL_PLAYER_INFO)
    // {
    //     printf("(debug) GLOBAL_PLAYER_INFO\n");
    // }
    // else if (query->m_header.type == RESPONSE_UUID)
    // {
    //     printf("(debug) RESPONSE_UUID\n");
    // }
    if (query->m_header.type == SOME_ONE_JOIN)
    {
        printf("(debug) SOME_ONE_JOIN\n");
    }
    else if (query->m_header.type == SOME_ONE_QUIT)
    {
        printf("(debug) SOME_ONE_QUIT\n");
    }
    else if (query->m_header.type == GAME_UPDATE)
    {
        printf("(debug) GAME_UPDATE\n");
    }
    else if (query->m_header.type == PLAYER_INFO_CERT)
    {
        printf("(debug) PLAYER_INFO_CERT\n");
    }
    else if (query->m_header.type == CLIENT_READY)
    {
        printf("(debug) CLIENT_READY\n");
    }

    // 如果玩家当前有未完成的发送数据 
    if (player->havent_send > 0)
    {
        //  将新的数据追加到发送缓存的未发送部分后面 
        memcpy(player->snd_buffer + player->havent_send, data, data_size);
        player->havent_send += data_size;   // 更新缓存之中的未发送数据大小
        return;
    }

    // 尝试直接发送数据
    ssize_t have_sent = write(target_sock, data, data_size);

    // 发送成功
    if (have_sent > 0)
    {
        remain_size = data_size - have_sent;    // 计算还没有发送的数据大小
        // printf("Sent %zd bytes\n", bytes_sent);
    }
    else if (have_sent == 0 || (have_sent < 0 && errno == EAGAIN))
    {
        // 检查是否所有数据都发送成功
        if (remain_size > 0)
        { /*仍然有数据没有被发送出去，登记到epoll上*/
            struct epoll_event ev;
            ev.events = EPOLLOUT | EPOLLET; // 不使用EPOLLONESHOOT
            ev.data.fd = target_sock;
            epoll_ctl(g_send_epoll_fd, EPOLL_CTL_MOD, target_sock, &ev);

            // 将还没有发送完的数据移动到对应player_info的send_buffer中
            memcpy(player->snd_buffer, data + have_sent, remain_size);
            player->havent_send = remain_size;

            g_is_write_eagain = true;
            not_right++;
        }
        else
        { /*所有数据都已经发送出去*/
            // All data sent, no need to register for EPOLLOUT
            printf("All data sent\n");
        }
    }
    else
    {
        // Handle error
        perror("send");
    }
}