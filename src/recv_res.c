#include "recv_res.h"
#include "util.h"

void *recv_res_main(void *)
{
    // 这里定义了一个 epoll_event 数组 ep_evt，用来存储 epoll_wait 返回的事件信息。
    struct epoll_event ep_evt[MAX_EPOLL_EVENT];

    // 函数的核心逻辑是一个循环，不断调用 epoll_wait，直到全局变量 g_over 被置为真才退出循环
    while (!g_over)
    {
        // epoll_wait 函数监听文件描述符 g_recv_epoll_fd，
        // 最多返回 MAX_EPOLL_EVENT 个就绪事件，超时时间为 TIME_OUT 毫秒。
        int ready_num = epoll_wait(g_recv_epoll_fd, ep_evt, MAX_EPOLL_EVENT, TIME_OUT);
        // ready_num 是就绪事件的数量，如果返回值大于 0，表示有可处理的事件；
        // 如果返回 0，则表示超时；如果返回 -1，则发生了错误。
        printf("recv event num: %d.\n", ready_num);

        // if (ready_num > 0)
        // {
        //     printf("%s", "recv_res_main: epoll_wait success.");
        //     printf("--recv event num: %d.--", ready_num);
        //     printCurrentTime();
        // }

        // for 循环会遍历所有返回的就绪事件，并判断是否是读事件（EPOLLIN）。读事件表示文件描述符有数据可读。
        for (int i = 0; i < ready_num; i++)
        { /*循环处理每个就绪的事件*/
            if (ep_evt[i].events & EPOLLIN)
            { /*读事件*/
                if (ep_evt[i].data.fd == g_pconf->listen_socket)
                { /*有新的连接请求*/
                    int result;
                    while (0 == (result = CQuery_accept_tcp_connect(g_pconf->listen_socket)))
                        ;
                    printAcceptError(result);
                } /*有新的连接请求*/
                else
                { /*有新的数据可读*/
                    // printf("\033[32m%s\033[0m", "recv_res_main: recv new message.\n");
                    CQuery_recv_message(ep_evt[i].data.fd);
                } /*有新的数据可读*/
            }
            else
            {
                printf("\033[31m%s\033[0m", "epoll_wait error.");
                printCurrentTime();
                printf("\033[31m%s\033[0m", "debug: error occured in recv_res_main.\n\n");
                perror("perror: epoll_wait error.");
            }
        }
    }

    return NULL;
}