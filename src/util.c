#include "util.h"

void generate_uuid(char *uuid)
{
    FILE *file = fopen("/proc/sys/kernel/random/uuid", "r");
    if (file == NULL)
    {
        perror("Error opening /proc/sys/kernel/random/uuid");
        exit(EXIT_FAILURE);
    }

    if (fscanf(file, "%s", uuid) != 1)
    {
        perror("Error reading UUID");
        fclose(file);
        exit(EXIT_FAILURE);
    }

    fclose(file);
}

int setnonblocking(int sockfd)
{
    // 实现设置 sockfd 为非阻塞模式的代码

    int opts;
    opts = fcntl(sockfd, F_GETFL, 0);
    if (opts < 0)
    {
        perror("fcntl(sock,GETFL)");
        return -1;
    }
    opts = opts | O_NONBLOCK;
    if (fcntl(sockfd, F_SETFL, opts) < 0)
    {
        perror("fcntl(sock,SETFL,opts)");
        return -1;
    }
}

void print_addr_info(struct sockaddr_in *addr)
{
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(addr->sin_addr), ip, INET_ADDRSTRLEN);
    printf("%s:%d ", ip, ntohs(addr->sin_port));
}

int config_socket(int sockfd)
{
    int one = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char *)&one, sizeof(int)) < 0)
        return SOCKET_CONFIGUE_ERROR;

    one = 1;
    if (setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (char *)&one, sizeof(int)) < 0)
        return SOCKET_CONFIGUE_ERROR;

    struct linger m_linger;
    m_linger.l_onoff = 1;
    m_linger.l_linger = 0;
    if (setsockopt(sockfd, SOL_SOCKET, SO_LINGER, (const char *)&m_linger, sizeof(m_linger)) < 0) /*设置socket参数：如关闭(closesocket()调用已执行)时有未发送数据，则逗留。*/
        return SOCKET_CONFIGUE_ERROR;

    one = 1;
    int keep_idle = KEEP_IDLE;
    int keep_interval = KEEP_INTERVAL;
    int keep_count = KEEP_COUNT;

    if (setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, (char *)&one, sizeof(int)) < 0)
        return SOCKET_CONFIGUE_ERROR;
    if (setsockopt(sockfd, SOL_TCP, TCP_KEEPIDLE, (char *)&keep_idle, sizeof(int)) < 0)
        return SOCKET_CONFIGUE_ERROR;
    if (setsockopt(sockfd, SOL_TCP, TCP_KEEPINTVL, (char *)&keep_interval, sizeof(int)) < 0)
        return SOCKET_CONFIGUE_ERROR;
    if (setsockopt(sockfd, SOL_TCP, TCP_KEEPCNT, (char *)&keep_count, sizeof(int)) < 0)
        return SOCKET_CONFIGUE_ERROR;

    if (0 > setnonblocking(sockfd))
        return SOCKET_CONFIGUE_ERROR;

    return NO_ERROR;
}

void printCurrentTime()
{
    // 获取当前时间
    time_t currentTime;
    time(&currentTime);

    // 转换为本地时间
    struct tm *localTime = localtime(&currentTime);

    // 格式化输出
    char timeString[100]; // 存储格式化后的时间字符串
    strftime(timeString, sizeof(timeString), "[%Y-%m-%d %H:%M:%S]", localTime);

    // 打印当前时间(使用橙色)
    printf("\033[0;33m");
    printf("%s\n", timeString);
    printf("\033[0m");
}

void printSocketInfo(int sockfd)
{
    struct sockaddr_in localAddr, remoteAddr;
    socklen_t addrLen = sizeof(struct sockaddr_in);

    // 获取本地地址信息
    if (getsockname(sockfd, (struct sockaddr *)&localAddr, &addrLen) == 0)
    {
        printf("Local IP address: %s\n", inet_ntoa(localAddr.sin_addr));
        printf("Local port       : %d\n", ntohs(localAddr.sin_port));
    }
    else
    {
        perror("getsockname");
        exit(EXIT_FAILURE);
    }

    // 获取远程地址信息
    if (getpeername(sockfd, (struct sockaddr *)&remoteAddr, &addrLen) == 0)
    {
        printf("Remote IP address: %s\n", inet_ntoa(remoteAddr.sin_addr));
        printf("Remote port       : %d\n", ntohs(remoteAddr.sin_port));
    }
    else
    {
        perror("getpeername");
        exit(EXIT_FAILURE);
    }
}

void printAcceptError(int result)
{
    if (result < 0 && result != RESOURCE_TEMPOREARILY_UNAVAILABLE)
    {
        if (result == RESOURCE_UNAVAILABLE)
        {
            printf("\033[31m%s\033[0m", "get_free_query error.");
            printCurrentTime();
            printf("\033[31m%s\033[0m", "debug: error occured in recv_res_main.\n");
            perror("perror: get_free_query error.");
        }
        else if (result == SOCKET_ACCEPT_ERROR)
        {
            printf("\033[31m%s\033[0m", "CQuery_accept_tcp_connect error.");
            printCurrentTime();
            printf("\033[31m%s\033[0m", "debug: error occured in recv_res_main.\n");
            perror("perror: CQuery_accept_tcp_connect error.");
        }
        else if (result == SOCKET_CONFIGUE_ERROR)
        {
            printf("\033[31m%s\033[0m", "config_socket error.");
            printCurrentTime();
            printf("\033[31m%s\033[0m", "debug: error occured in recv_res_main.\n");
            perror("perror: config_socket error.");
        }
        else if (result == EPOLL_ERROR)
        {
            printf("\033[31m%s\033[0m", "epoll_ctl error.");
            printCurrentTime();
            printf("\033[31m%s\033[0m", "debug: error occured in recv_res_main.\n\n");
            perror("perror: epoll_ctl error.");
        }
    }
}