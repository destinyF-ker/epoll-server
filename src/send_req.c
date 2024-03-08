/*FileName: send_req.cpp*/
#include "send_req.h"

void *send_req_main(void *)
{
    not_right = 0;
    int tmp_count = 1;
    CQuery *pQuery = NULL;
    struct epoll_event ep_evt[MAX_EPOLL_EVENT];

    while (!g_over)
    {
        // printf("\033[32m%s\033[0m, not_right: %d\n", "send_req_main: ", not_right);
        if (g_is_write_eagain)
        {
            int ready_num = epoll_wait(g_send_epoll_fd, ep_evt, MAX_EPOLL_EVENT, 0); /*等待事件*/

            for (int i = 0; i < ready_num; i++)
            {
                int sockfd = ep_evt[i].data.fd;
                player_info *player = get_player_info_by_sock(sockfd);
                if (player == NULL)
                {
                    perror("get_player_info_by_sock");
                    continue;
                }
                if (!player->available)
                    continue;
                int have_write = write(sockfd, player->snd_buffer, player->havent_send);
                if (have_write > 0)
                {
                    player->havent_send -= have_write;
                    if (player->havent_send > 0)
                    {
                        // 将还没有发送完的数据移动到对应player_info的send_buffer中
                        memmove(player->snd_buffer, player->snd_buffer + have_write, player->havent_send);
                    }
                    else
                    {
                        // 将该socket从epoll中移除
                        epoll_ctl(g_send_epoll_fd, EPOLL_CTL_DEL, sockfd, NULL);
                        not_right--;
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

            if (not_right == 0)
                g_is_write_eagain = false;
        }
        if (NULL == (pQuery = get_gwork_query()))
            continue;

        player_info *player = get_player_info_by_sock(pQuery->m_socket_fd);
        if (player == NULL)
        {
            perror("get_player_info_by_sock");
            continue;
        }
        minus_message_count(player);
        if (!player->available)
        {
            add_free_list(pQuery);
            continue;
        }

        if (pQuery->m_header.type == GAME_UPDATE ||
            pQuery->m_header.type == SOME_ONE_JOIN ||
            pQuery->m_header.type == SOME_ONE_QUIT)
        {
            if (get_player_info_array_length() > 1)
                handle_group_send(pQuery);
            else
            {
                printf("(debug) %s\n", "send_req_main: no player info.");
            }
        }
        else
        {
            printf("send_req_main: %d:%d\n", pQuery->m_header.type, tmp_count++);
            handle_send(pQuery->m_socket_fd, pQuery);
        }

        add_free_list(pQuery);
    }
}

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

void handle_send(int target_sock, CQuery *query)
{
    printf("\033[32m%s\033[0m\n", "handle_send");
    char *data = query->m_byte_Query;
    int data_size = query->m_query_len;
    int remain_size = data_size;

    player_info *player = get_player_info_by_sock(target_sock);
    if (player == NULL)
    {
        perror("get_player_info_by_sock");
        return;
    }

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

    if (player->havent_send > 0)
    {
        // 将还没有发送完的数据移动到对应player_info的send_buffer中
        memcpy(player->snd_buffer + player->havent_send, data, data_size);
        player->havent_send += data_size;
        return;
    }

    ssize_t have_sent = write(target_sock, data, data_size);

    if (have_sent > 0)
    {
        remain_size = data_size - have_sent;
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