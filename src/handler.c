#include "handler.h"

void *event_handler_main(void *)
{
    CQuery *query = NULL;
    while (!g_over)
    {
        // printf("\033[32m%s\033[0m\n", "event_handler_main: ");
        if (NULL == (query = get_gready_query()))
            continue;
        printf("\033[32m%s\033[0m %s type: %d\n", "(server)", "event_handler_main: get_gready_query success.", query->m_header.type);
        if (query->m_header.type != RESPONSE_UUID)
        {
            player_info *player = get_player_info_by_sock(query->m_socket_fd);
            if (player == NULL)
            {
                perror("get_player_info_by_sock");
                continue;
            }
            plus_message_count(player);
        }

        switch (query->m_header.type)
        {
        case RESPONSE_UUID:
            handle_response_uuid(query);
            break;
        case SOME_ONE_QUIT:
            printf("\033[32m%s\033[0m\n", "event_handler_main: SOME_ONE_QUIT");
            handle_some_one_quit(query);
            break;
        case GAME_UPDATE:
            handle_game_update(query);
            break;
        case PLAYER_INFO_CERT:
            handle_player_info_cert(query);
            break;
        case CLIENT_READY:
            handle_client_ready(query);
            break;
        default:
            break;
        }
    }
}

void handle_response_uuid(CQuery *query)
{
    // printf("%s\n", "handle_response_uuid: ");
    add_player_info(query->m_byte_Query, query->m_socket_fd);
    CQuery_pack_message(query);
    add_gwork_list(query);
}

void handle_global_player_info(CQuery *query)
{
    // player_info *current = player_infos.head;
    // while (current != NULL)
    // {
    //     printf("======\n");
    //     printf("(debug) %s\n", "handle_global_player_info: ");
    //     printf("(debug) id: %s\n", current->id);
    //     printf("(debug) available: %d\n", current->available);
    //     printf("(debug) message_count: %d\n", current->message_count);
    //     printf("======\n");
    //     current = current->next;
    // }

    scan_and_delete_unavailable_player_info();
    query->m_header.type = GLOBAL_PLAYER_INFO;
    char *buffer = package_global_player_info(query->m_socket_fd);
    if (NULL == buffer) // 没有玩家信息
    {
        printf("(debug) %s\n", "handle_global_player_info: no player info.");
        query->m_query_len = 0;
        CQuery_pack_message(query);
        add_gwork_list(query);
        return;
    }
    CQuery_set_query_buffer(query, buffer, strlen(buffer) - 1);
    free(buffer);

    CQuery_pack_message(query);

    add_gwork_list(query);
}

void handle_some_one_join(CQuery *query)
{
    printf("(debug)\033[32m%s\033[0m\n", "handle_some_one_join: ");
    query->m_header.type = SOME_ONE_JOIN;
    // 把@去掉
    query->m_query_len--;
    CQuery_pack_message(query);
    add_gwork_list(query);
}

void handle_some_one_quit(CQuery *query)
{
    CQuery_pack_message(query);
    add_gwork_list(query);
}

void handle_game_update(CQuery *query)
{
    CQuery_pack_message(query);
    add_gwork_list(query);
}

void handle_player_info_cert(CQuery *query)
{
    handle_global_player_info(query);
}

void handle_client_ready(CQuery *query)
{
    printf("(debug)\033[32m%s\033[0m, query len: %d\n", "handle_client_ready", query->m_query_len);

    // 从query中获取玩家的id和name
    // 将玩家的id和name加入到player_infos中
    char *id = (char *)malloc(sizeof(char) * (PLAYER_ID_LEN + 1));
    char *name = (char *)malloc(sizeof(char) * (MAX_PLAYER_NAME_LEN + 1));
    memmove(id, query->m_byte_Query, PLAYER_ID_LEN);
    id[PLAYER_ID_LEN] = '\0';

    int name_len = 0;
    while (query->m_byte_Query[PLAYER_ID_LEN + name_len] != '@')
        name_len++;

    memmove(name, query->m_byte_Query + PLAYER_ID_LEN, name_len);
    name[name_len] = '\0';

    set_player_name(id, name);

    handle_some_one_join(query);
}