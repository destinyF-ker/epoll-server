#ifndef __HANDLER_H__
#define __HANDLER_H__

#include <sys/time.h>
#include <unistd.h>
#include <sys/epoll.h>
#include "query_list.h"
#include "server_conf.h"
#include "config.h"

extern int g_send_epoll_fd;
extern pconf_t *g_pconf;
extern bool g_over;

void *event_handler_main(void *);
void handle_response_uuid(CQuery *query);
void handle_global_player_info(CQuery *query);
void handle_some_one_join(CQuery *query);
void handle_some_one_quit(CQuery *query);
void handle_game_update(CQuery *query);
void handle_player_info_cert(CQuery *query);
void handle_client_ready(CQuery *query);

#endif