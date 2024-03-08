#include "recv_res.h"
#include "util.h"

void *recv_res_main(void *)
{
    struct epoll_event ep_evt[MAX_EPOLL_EVENT];

    while (!g_over)
    {
        int ready_num = epoll_wait(g_recv_epoll_fd, ep_evt, MAX_EPOLL_EVENT, TIME_OUT);
        printf("recv event num: %d.\n", ready_num);

        // if (ready_num > 0)
        // {
        //     printf("%s", "recv_res_main: epoll_wait success.");
        //     printf("--recv event num: %d.--", ready_num);
        //     printCurrentTime();
        // }

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