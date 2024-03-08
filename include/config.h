#ifndef CONFIG_H
#define CONFIG_H

#define MAX_PLAYER_NUM 10
#define MAX_QUERY_NUM 5000
#define INET_ADDRSTRLEN 16

#define UNIT_BUFFER_SIZE 1024
#define QUERY_BUFFER_LEN 512

#define MAX_PLAYER_NAME_LEN 32
#define PLAYER_ID_LEN 36

#define MAX_EPOLL_EVENT 500

#define NO_ERROR 0
#define RESOURCE_TEMPOREARILY_UNAVAILABLE -1
#define RESOURCE_UNAVAILABLE -2
#define SOCKET_CONFIGUE_ERROR -3
#define SOCKET_ACCEPT_ERROR -4
#define EPOLL_ERROR -5

#define KEEP_IDLE 60
#define KEEP_INTERVAL 5
#define KEEP_COUNT 3

#endif