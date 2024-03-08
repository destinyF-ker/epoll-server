#ifndef __PLAYER_INFO_ARRAY_H__
#define __PLAYER_INFO_ARRAY_H__

#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include "query.h"

typedef struct info_table
{
    char *id;
    char *name;
    int socketfd;

    char rcv_buffer[UNIT_BUFFER_SIZE * 10];
    CQuery *query;
    bool is_header_handled;
    int prepare_to_handle;
    int havent_handle;

    char snd_buffer[UNIT_BUFFER_SIZE * 10];
    int havent_send;

    bool available;
    bool ready;
    int message_count;
    pthread_mutex_t *msg_count_mutex;

    struct info_table *next;
} player_info;

typedef struct
{
    size_t length;
    player_info *head;
} player_info_array;

extern player_info_array player_infos;

void init_player_info_array_lock();

void player_info_array_init();

int get_player_info_array_length();

int add_player_info(const char *id, int socketfd);

int set_player_name(char *id, char *name);

int delete_player_info(const char *id);

int delete_player_info_by_socketfd(int socketfd);

int clear_player_info_array();

player_info *get_player_info_by_sock(int socketfd);

int *get_player_info_sockfds();

char *package_global_player_info(int except_socketfd);

int scan_and_delete_unavailable_player_info();

void plus_message_count(player_info *info);

void minus_message_count(player_info *info);

int get_message_count(player_info *info);
#endif