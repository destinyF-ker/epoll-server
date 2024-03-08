#include "server_conf.h"
#include "util.h"
#include <stdio.h>

int load_config(pconf_t *pconf, uint16_t port)
{
    pconf->port = port;

    int listen_socket = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addbuf;
    addbuf.sin_family = AF_INET;
    // 使用htons函数将端口号转换为网络字节序·
    addbuf.sin_port = htons(port);
    // 该字段表示套接字绑定的IP地址。
    // INADDR_ANY 是一个常量，表示将套接字绑定到所有可用的本地网络接口上。
    addbuf.sin_addr.s_addr = INADDR_ANY;

    if (-1 == bind(listen_socket, (struct sockaddr *)&addbuf, sizeof(addbuf)))
    {
        perror("server can't bind!");
        return -1;
    }

    listen(listen_socket, 1);
    config_socket(listen_socket);
    pconf->listen_socket = listen_socket;

    // 使用绿色打印
    printf("\033[32m");
    printf("server config success!, listening to socket %d\n", pconf->listen_socket);
    printf("\033[0m");

    return 0;
}