#ifndef UTIL_H
#define UTIL_H

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <fcntl.h>
#include <time.h>

#include "config.h"

void generate_uuid(char *uuid);
int setnonblocking(int sockfd);
int config_socket(int sockfd);
void print_addr_info(struct sockaddr_in *addr);
void printCurrentTime();
void printSocketInfo(int socketfd);
void printAcceptError(int result);

#endif