#ifndef POINT_CONF_H
#define POINT_CONF_H

#include "config.h"
#include <stdbool.h>
#include <stdint.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <error.h>

typedef struct
{
    int listen_socket; /*服务器监听socket*/
    uint16_t port;     /*服务器挂载端口*/
} pconf_t;

int load_config(pconf_t *pconf, uint16_t port);

#endif