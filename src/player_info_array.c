#include "player_info_array.h"

pthread_mutex_t player_info_array_mutex;

void init_player_info_array_lock()
{
    pthread_mutex_init(&player_info_array_mutex, NULL);
}

void player_info_array_init()
{
    player_infos.length = 0;
    player_infos.head = NULL;
}

int get_player_info_array_length()
{
    pthread_mutex_lock(&player_info_array_mutex);
    int length = player_infos.length;
    pthread_mutex_unlock(&player_info_array_mutex);

    return length;
}

/**
 * @brief 添加新的玩家信息到玩家信息链表中。
 *
 * 该函数为新玩家创建一个 `player_info` 结构体，并将其添加到全局链表 `player_infos` 的头部。新玩家信息包括
 * 玩家ID、socket文件描述符、消息计数等。该函数使用互斥锁来确保对链表操作的线程安全。
 *
 * @param id 新玩家的唯一标识符，传入时为一个常量字符串。
 * @param socketfd 与该玩家通信的 socket 文件描述符。
 * @return int 操作结果：
 * - 0：成功添加玩家信息。
 * - -1：内存分配失败。
 *
 * @note 在执行过程中，函数会为 `player_info` 结构体及其相关字段（如 `id` 和 `msg_count_mutex`）分配内存。
 * 如果分配内存失败，函数会进行相应的清理并返回错误码。使用互斥锁 `player_info_array_mutex` 确保线程安全。
 */
int add_player_info(const char *id, int socketfd)
{
    // printf("add_player_info: %s\n", id);
    pthread_mutex_lock(&player_info_array_mutex);
    player_info *new_player_info = malloc(sizeof(player_info));
    if (new_player_info == NULL)
    {
        pthread_mutex_unlock(&player_info_array_mutex);
        return -1;
    }

    // 为新的player_info分配内存
    new_player_info->id = malloc(PLAYER_ID_LEN + 1);
    if (new_player_info->id == NULL)
    {
        free(new_player_info);
        pthread_mutex_unlock(&player_info_array_mutex);
        return -1;
    }
    strcpy(new_player_info->id, id);

    // 初始化玩家信息
    new_player_info->socketfd = socketfd;
    new_player_info->message_count = 1;
    new_player_info->havent_send = 0;
    new_player_info->available = true;
    new_player_info->ready = false;
    new_player_info->msg_count_mutex = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(new_player_info->msg_count_mutex, NULL);

    // 将新玩家添加到链表头部 
    new_player_info->next = player_infos.head;
    player_infos.head = new_player_info;
    player_infos.length++;

    pthread_mutex_unlock(&player_info_array_mutex);

    // printf("done\n");
    return 0;
}

/**
 * @brief 设置指定玩家的名称，并标记玩家为已准备好状态。
 *
 * 该函数通过遍历玩家信息链表，查找具有指定 `id` 的玩家，并为其设置名称。设置成功后，将玩家的 `ready` 状态
 * 标记为 `true`。该函数使用互斥锁来保证线程安全。
 *
 * @param id 玩家唯一标识符字符串。假定在外部已经分配内存，需要在函数内释放。
 * @param name 要设置的玩家名称。假定在外部已经分配内存，需要在函数内释放。
 * @return int 操作结果：
 * - 0：成功设置名称。
 * - -1：内存分配失败。
 * - -2：未找到匹配的玩家。
 *
 * @note 该函数在处理完毕后会释放 `id` 和 `name` 的内存，并解锁互斥锁。
 */
int set_player_name(char *id, char *name)
{
    pthread_mutex_lock(&player_info_array_mutex);
    player_info *current = player_infos.head;
    while (current != NULL)
    {
        if (strcmp(current->id, id) == 0)
        {
            current->name = malloc(strlen(name) + 1);
            if (current->name == NULL)
            {
                pthread_mutex_unlock(&player_info_array_mutex);
                return -1;
            }
            strcpy(current->name, name);
            current->ready = true;
            free(id);
            free(name);
            pthread_mutex_unlock(&player_info_array_mutex);
            return 0;
        }
        current = current->next;
    }
    free(id);
    free(name);
    pthread_mutex_unlock(&player_info_array_mutex);
    return -2;
}

int delete_player_info(const char *id)
{
    pthread_mutex_lock(&player_info_array_mutex);
    player_info *current = player_infos.head;
    player_info *previous = NULL;
    while (current != NULL)
    {
        if (strcmp(current->id, id) == 0)
        {
            if (previous == NULL)
            {
                player_infos.head = current->next;
            }
            else
            {
                previous->next = current->next;
            }
            free(current->id);
            free(current->name);
            free(current);
            player_infos.length--;
            return 0;
        }
        previous = current;
        current = current->next;
    }
    pthread_mutex_unlock(&player_info_array_mutex);
    return 1;
}

int delete_player_info_by_socketfd(int socketfd)
{
    pthread_mutex_lock(&player_info_array_mutex);
    player_info *current = player_infos.head;
    player_info *previous = NULL;
    while (current != NULL)
    {
        if (current->socketfd == socketfd)
        {
            if (previous == NULL)
            {
                player_infos.head = current->next;
            }
            else
            {
                previous->next = current->next;
            }
            free(current->id);
            free(current->name);
            free(current);
            player_infos.length--;
            return 0;
        }
        previous = current;
        current = current->next;
    }
    pthread_mutex_unlock(&player_info_array_mutex);
    return 1;
}

int clear_player_info_array()
{
    player_info *current = player_infos.head;
    player_info *next;
    while (current != NULL)
    {
        next = current->next;
        free(current->id);
        free(current->name);
        free(current);
        current = next;
    }
    player_infos.length = 0;
    player_infos.head = NULL;
    return 0;
}

// ---------------------------------------------------

/**
 * @brief 根据 socket 文件描述符查找并返回对应的玩家信息。
 *
 * 该函数通过遍历全局玩家信息链表，根据传入的 `socketfd` 查找匹配的玩家信息，并返回该玩家的 `player_info` 结构体指针。
 * 如果找到匹配的玩家，返回该玩家的指针；如果未找到，返回 `NULL`。函数在操作过程中使用互斥锁确保线程安全。
 *
 * @param socketfd 要查找的玩家的 socket 文件描述符。
 * @return player_info* 指向匹配玩家的 `player_info` 结构体的指针；如果未找到匹配的玩家，返回 `NULL`。
 *
 * @note 该函数会锁定 `player_info_array_mutex` 互斥锁，确保链表访问的线程安全。在查找结束后，无论是否找到玩家，都会解锁。
 */
player_info *get_player_info_by_sock(int socketfd)
{
    pthread_mutex_lock(&player_info_array_mutex);
    player_info *current = player_infos.head;
    while (current != NULL)
    {
        if (current->socketfd == socketfd)
        {
            pthread_mutex_unlock(&player_info_array_mutex);
            return current;
        }
        current = current->next;
    }
    pthread_mutex_unlock(&player_info_array_mutex);
    return NULL;
}

/**
 * @brief 获取当前所有玩家的 socket 文件描述符数组。
 *
 * 该函数遍历全局玩家信息链表，并返回一个包含所有玩家 `socketfd` 的动态数组。数组的第一个元素 `sockfds[0]` 
 * 存储了数组中元素的数量，其后的元素为玩家的 socket 文件描述符。若玩家列表为空或内存分配失败，则返回 `NULL`。
 * 函数使用互斥锁确保链表访问的线程安全。
 *
 * @return int* 指向包含玩家 `socketfd` 的动态数组指针，数组的 0 号位置存储了数组的长度。
 * 返回 `NULL` 表示玩家列表为空或内存分配失败。
 *
 * @note 返回的数组由 `malloc` 分配内存，调用方需要在使用完毕后调用 `free` 释放内存。该函数使用 `player_info_array_mutex` 
 * 互斥锁来确保线程安全，在完成链表操作后会解锁。
 */
int *get_player_info_sockfds()
{
    pthread_mutex_lock(&player_info_array_mutex);
    if (player_infos.length == 0)
    {
        pthread_mutex_unlock(&player_info_array_mutex);
        return NULL;
    }
    int *sockfds = malloc(sizeof(int) * player_infos.length + 1);
    if (sockfds == NULL)
    {
        pthread_mutex_unlock(&player_info_array_mutex);
        return NULL;
    }
    player_info *current = player_infos.head;
    int i = 1;
    while (current != NULL)
    {
        sockfds[i] = current->socketfd;
        current = current->next;
        i++;
    }
    // 数组的0号元素之中存储了数组之中元素的数量
    sockfds[0] = player_infos.length;
    pthread_mutex_unlock(&player_info_array_mutex);
    return sockfds;
}

/**
 * @brief 打包全局玩家信息为一个字符串，排除特定的 socket 文件描述符。
 *
 * 该函数遍历全局玩家信息链表，排除 `except_socketfd` 对应的玩家，将其他玩家的 `id` 和 `name` 连接成一个字符串，
 * 每个玩家的信息之间用 `@` 分隔。返回的字符串以 `\0` 结尾，并由 `malloc` 动态分配内存。
 * 如果玩家数量为 1 或 0，或者内存分配失败，则返回 `NULL`。函数使用互斥锁确保线程安全。
 *
 * @param except_socketfd 需要排除的 socket 文件描述符，该玩家信息不会包含在返回的字符串中。
 * @return char* 包含全局玩家信息的字符串，以 `\0` 结尾。若发生错误返回 `NULL`。
 *
 * @note 返回的字符串由 `malloc` 分配内存，调用方在使用完毕后需要调用 `free` 释放内存。
 * 函数使用 `player_info_array_mutex` 互斥锁来确保链表访问的线程安全，在完成链表操作后会解锁。
 */
char *package_global_player_info(int except_socketfd)
{
    pthread_mutex_lock(&player_info_array_mutex);
    player_info *info = player_infos.head;
    int have_set_len = 0;
    char *delimiter = "@";
    char *global_player_info = malloc(QUERY_BUFFER_LEN);
    // int max_buffer_size = UNIT_BUFFER_SIZE * 2;

    // 检查内存分配是否成功，以及玩家数量是否大于 1（否则无需打包信息）
    if (global_player_info == NULL ||
        player_infos.length == 1 || player_infos.length == 0)
    {
        pthread_mutex_unlock(&player_info_array_mutex);
        free(global_player_info);   // 如果内存分配失败，则释放已经分配的内存 
        return NULL;
    }
    // 遍历所有玩家信息，将符合条件的玩家信息拼接成字符串
    while (info)
    {
        // 排除 `except_socketfd` 的玩家，并且确保玩家状态为 `available`
        if (info->socketfd != except_socketfd && info->available)
        {
            // if (have_set_len + strlen(info->id) + strlen(info->name) + strlen(delimiter) > max_buffer_size)
            // {
            //     max_buffer_size += UNIT_BUFFER_SIZE;
            //     global_player_info = realloc(global_player_info, max_buffer_size);
            //     if (global_player_info == NULL)
            //     {
            //         pthread_mutex_unlock(&player_info_array_mutex);
            //         // TODO 这里服务器应该是崩溃了，需不需要传递这个消息给客户端？
            //         return NULL;
            //     }
            // }

            // printf("(debug)strlen id: %d\n", strlen(info->id));
            // printf("(debug)strlen name: %d\n", strlen(info->name));
            // printf("(debug)strlen delimiter: %d\n", strlen(delimiter));
            memmove(global_player_info + have_set_len, info->id, strlen(info->id));
            have_set_len += strlen(info->id);
            memmove(global_player_info + have_set_len, info->name, strlen(info->name));
            have_set_len += strlen(info->name);
            memmove(global_player_info + have_set_len, delimiter, strlen(delimiter));
            have_set_len += strlen(delimiter);
        }
        info = info->next;
    } /*将所有的玩家信息拼接成一个字符串，用@分隔*/
    global_player_info[have_set_len] = '\0';
    pthread_mutex_unlock(&player_info_array_mutex);

    printf("(debug)global_player_info: %s\n", global_player_info);
    return global_player_info;
}

/**
 * @brief 扫描并删除不可用的玩家信息
 *
 * 该函数遍历 `player_infos` 链表，查找所有不可用且消息计数为 0 的玩家信息，
 * 并将其从链表中删除，释放相关资源。删除玩家信息的条件为：
 * - `available` 标志为假（表示玩家不可用）。
 * - `message_count` 为 0（表示没有未处理的消息）。
 *
 * 删除的玩家信息包括以下资源：
 * - 玩家 ID 和名字的内存。
 * - 玩家在接收和发送 epoll 中注册的 socket 文件描述符。
 * - 玩家 socket 连接。
 * - 互斥锁（用于管理消息计数的同步机制）。
 *
 * 该函数会对 `player_infos` 链表加锁，以防止其他线程同时对链表进行修改。
 * 删除操作完成后会更新链表的长度，并释放内存。
 *
 * @return int
 * 函数总是返回 0，表示操作成功。
 *
 * @note 
 * - 该函数的执行依赖于以下全局变量：
 *   - `player_infos`: 玩家信息链表。
 *   - `g_recv_epoll_fd`: 接收 epoll 文件描述符。
 *   - `g_send_epoll_fd`: 发送 epoll 文件描述符。
 * - 该函数会调用 `epoll_ctl`、`close` 等函数来操作 epoll 和 socket 资源。
 */
int scan_and_delete_unavailable_player_info()
{
    pthread_mutex_lock(&player_info_array_mutex);   // 加锁，防止并发修改 player_info 列表
    player_info *current = player_infos.head;   // 初始化遍历指针 `current`，指向 player_info 链表的头部
    player_info *previous = NULL;   // `previous` 用于记录 `current` 的前一个节点
    player_info *tmp = NULL;    // 临时指针 `tmp` 用于删除节点后释放内存
    bool delete_flag = false;   // 删除标志位，用于指示当前节点是否需要删除

    while (current != NULL)
    {
        // 判断当前 player 是否不可用，并且消息队列为空
        if (!current->available && current->message_count == 0)
        {
            printf("(debug)prepare to delete unavailable player info\n");
            if (previous == NULL)
                player_infos.head = current->next;
            else
                previous->next = current->next;
            
            // 释放当前 player 的 ID 和名字内存
            free(current->id);
            free(current->name);
            // 从接收 epoll 中删除该玩家的 socket 描述符
            epoll_ctl(g_recv_epoll_fd, EPOLL_CTL_DEL, current->socketfd, NULL);
            // 如果该玩家还有未发送的消息，从发送 epoll 中删除其 socket 描述符
            if (current->havent_send > 0)
                epoll_ctl(g_send_epoll_fd, EPOLL_CTL_DEL, current->socketfd, NULL);
            // 关闭该玩家的 socket 连接
            close(current->socketfd);
            // 销毁该玩家的消息计数互斥锁，并释放互斥锁内存
            pthread_mutex_destroy(current->msg_count_mutex);
            free(current->msg_count_mutex);

            // 设置删除标志位
            delete_flag = true;
            player_infos.length--;
        }

        previous = current;
        tmp = current;
        current = current->next;

        // 如果删除标志位为真，释放当前节点的内存
        if (delete_flag)
        {
            delete_flag = false;
            free(tmp);
        }
    }
    pthread_mutex_unlock(&player_info_array_mutex);
    return 0;
}

void plus_message_count(player_info *info)
{
    printf("(debug) \033[33m%s\033[0m message count ++\n", info->id);
    pthread_mutex_lock(info->msg_count_mutex);
    info->message_count++;
    pthread_mutex_unlock(info->msg_count_mutex);
}

void minus_message_count(player_info *info)
{
    printf("(debug) \033[33m%s\033[0m message count --\n", info->id);
    pthread_mutex_lock(info->msg_count_mutex);
    info->message_count--;
    pthread_mutex_unlock(info->msg_count_mutex);
}

int get_message_count(player_info *info)
{
    pthread_mutex_lock(info->msg_count_mutex);
    int count = info->message_count;
    pthread_mutex_unlock(info->msg_count_mutex);
    return count;
}