#include "handler.h"

/**
 * @brief 事件处理主函数，运行在独立线程中。
 *
 * 该函数在全局标志 `g_over` 未被设置的情况下，持续从全局队列中通过 `get_gready_query()` 获取并处理轮转数据。
 *
 * 函数首先获取一个查询请求，并根据其类型执行相应的操作。如果查询类型不是 `RESPONSE_UUID`，则通过
 * `get_player_info_by_sock()` 获取对应玩家的信息，并使用 `plus_message_count()` 增加该玩家的消息计数。
 *
 * 根据查询的类型，函数将查询分发到不同的处理函数：
 * 
 * - `RESPONSE_UUID`: 调用 `handle_response_uuid(query)` 处理 UUID 响应。
 * - `SOME_ONE_QUIT`: 当有玩家退出时，记录日志并调用 `handle_some_one_quit(query)` 处理退出事件。
 * - `GAME_UPDATE`: 调用 `handle_game_update(query)` 处理游戏更新。
 * - `PLAYER_INFO_CERT`: 调用 `handle_player_info_cert(query)` 处理玩家认证信息。
 * - `CLIENT_READY`: 调用 `handle_client_ready(query)` 处理客户端准备就绪事件。
 * - 默认: 如果类型不属于以上情况，则忽略该查询。
 *
 * 该函数会一直运行，直到全局变量 `g_over` 被设置。
 * 
 * @param void* 线程兼容的未使用参数。
 * @return void* 返回值未使用。
 */
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

/**
 * @brief 处理客户端连接时的响应，初始化玩家信息并打包消息。
 *
 * 当客户端第一次连接到服务器时，服务器会创建一个半初始化的 `player_info` 结构体，
 * 以便后续操作使用。此时，TCP 连接已经建立，接下来处理自定义的二进制协议内容。
 *
 * @param query 包含客户端请求的请求对象，携带了消息缓冲区和 socket 文件描述符。
 *
 * 主要步骤：
 * 1. 调用 `add_player_info` 函数，使用 `query` 中的数据为连接创建 `player_info` 对象。
 * 2. 通过 `CQuery_pack_message` 函数，将消息头和消息缓冲区中的数据打包到 `query` 的缓冲区中。
 *    当前实现在缓冲区中为消息头预留空间，然后将头部放入缓冲区中，这部分的效率可能需要优化。
 * 3. 将处理完成的 `query` 放入工作队列中，以便稍后发送响应。
 */
void handle_response_uuid(CQuery *query)
{
    // 当客户端第一次连接到服务器之中时，服务器会先创建一个半初始化的 player_info
    // 方便接下来的操作，注意，此时 TCP 连接已经建立，只不过在进行我们定义的二进制协议之中的内容
    add_player_info(query->m_byte_Query, query->m_socket_fd);
    // 调用在 binary_protocol 之中编写的 oack_message 将 query 之中携带的消息头
    // 和其缓冲区之中的数据一起打包到其缓冲区之中（实际上就是在缓冲区之中腾出 header 的空间
    // 然后将 header 放进去，实在有点没有效率，后面我再想想怎么优化这一部分）
    CQuery_pack_message(query);
    // 现在这个 query 之中携带的数据就可以放到 work_list 之中等待发送了
    add_gwork_list(query);
}

/**
 * @brief 处理全局玩家信息的请求，扫描并删除无效玩家信息，返回当前有效玩家列表。
 *
 * 该函数主要处理客户端查询全局玩家信息的请求。首先会扫描并删除无效的玩家信息，
 * 然后将剩余的玩家信息打包发送回客户端。
 *
 * @param query 包含客户端请求的请求对象，携带了消息缓冲区和 socket 文件描述符。
 *
 * 主要步骤：
 * 1. 调用 `scan_and_delete_unavailable_player_info`，清理所有无效的玩家信息。
 * 2. 设置查询头部的消息类型为 `GLOBAL_PLAYER_INFO`。
 * 3. 调用 `package_global_player_info`，根据 `query` 的 socket 文件描述符获取当前所有玩家信息，
 *    并将其打包到缓冲区中。
 *    - 如果没有玩家信息可返回，设置查询长度为 0，打包消息并将查询对象放入工作队列，结束函数。
 * 4. 否则，将玩家信息设置到查询的缓冲区中，并打包消息。
 * 5. 将处理完成的 `query` 对象放入工作队列中，准备发送响应。
 */
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
        CQuery_pack_message(query); // ???当时我是怎么设计的？为什么这里还要发送一次？
        add_gwork_list(query);
        return;
    }
    // -1 就减在 '\0'
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

/**
 * @brief 处理客户端准备完成的请求，提取玩家信息并将其加入玩家列表。
 *
 * 该函数处理客户端发送的“准备完成”消息，从查询对象中提取玩家的 ID 和名称，加入全局玩家信息列表中，
 * 并广播该玩家的加入事件。
 *
 * @param query 包含客户端请求的查询对象，携带了消息缓冲区和 socket 文件描述符。
 *
 * 主要步骤：
 * 1. 从 `query` 的缓冲区中提取玩家的 ID 和名称。
 *    - 分配内存用于存储玩家的 ID 和名称，并分别将其拷贝到相应的内存位置。
 *    - 玩家 ID 的长度由宏 `PLAYER_ID_LEN` 定义，名称长度通过查找终止符 `@` 计算。
 * 2. 调用 `set_player_name` 函数，将玩家的 ID 和名称存入全局玩家信息列表。
 * 3. 调用 `handle_some_one_join` 函数，处理该玩家加入的后续逻辑。
 */
void handle_client_ready(CQuery *query)
{
    printf("(debug)\033[32m%s\033[0m, query len: %d\n", "handle_client_ready", query->m_query_len);

    // 从query中获取玩家的id和name
    // 将玩家的id和name加入到player_infos中
    char *id = (char *)malloc(sizeof(char) * (PLAYER_ID_LEN + 1));
    char *name = (char *)malloc(sizeof(char) * (MAX_PLAYER_NAME_LEN + 1));
    memmove(id, query->m_byte_Query, PLAYER_ID_LEN);
    id[PLAYER_ID_LEN] = '\0';

    // 计算玩家 name 的长度
    int name_len = 0;
    while (query->m_byte_Query[PLAYER_ID_LEN + name_len] != '@')
        name_len++;

    memmove(name, query->m_byte_Query + PLAYER_ID_LEN, name_len);
    name[name_len] = '\0';

    // 设置玩家的名称
    set_player_name(id, name);

    // 处理玩家加入事件 
    handle_some_one_join(query);
}