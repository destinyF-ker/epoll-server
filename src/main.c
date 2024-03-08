#include <boost_up.h>
#include <stdlib.h>
#include <stdio.h>
#include <binary_protocol.h>
#include <inttypes.h>

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("Usage: %s <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    int port = atoi(argv[1]);

    g_pconf = (pconf_t *)malloc(sizeof(pconf_t));
    if (0 != load_config(g_pconf, port))
    {
        perror("can't start server!");
        return -1;
    } /*初始化g_pconf变量*/

    int result = init_main();
    if (0 != result)
    {
        printf("\033[31m%s\033[0m", "(server)init_main error.");
        printCurrentTime();
        printf("\033[31m%s\033[0m", "debug: error occured in main.\n\n");
        switch (result)
        {
        case -1:
            printf("\033[31m%s\033[0m", "(server)malloc error.\n");
            break;
        case -2:
            printf("\033[31m%s\033[0m", "(server)epoll_create error.\n");
            break;
        }
        return -1;
    } /*初始化全局变量*/

    // 新建3个子线程，分别是处理线程，发包线程，收包线程
    pthread_t handler_pid;
    pthread_t send_pid;
    pthread_t recv_pid;
    if (0 != pthread_create(&handler_pid, NULL, event_handler_main, NULL))
    {
        clean_main();
        return -2;
    }
    if (0 != pthread_create(&send_pid, NULL, send_req_main, NULL))
    {
        clean_main();
        return -2;
    }
    if (0 != pthread_create(&recv_pid, NULL, recv_res_main, NULL))
    {
        clean_main();
        return -2;
    }

    pthread_join(handler_pid, NULL);
    pthread_join(send_pid, NULL);
    pthread_join(recv_pid, NULL);

    // if (0 != clean_main())
    // {
    //     return -3;
    // } /*清理全局变量*/

    return 0;
}