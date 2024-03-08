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

    new_player_info->socketfd = socketfd;
    new_player_info->message_count = 1;
    new_player_info->havent_send = 0;
    new_player_info->available = true;
    new_player_info->ready = false;
    new_player_info->msg_count_mutex = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(new_player_info->msg_count_mutex, NULL);

    new_player_info->next = player_infos.head;
    player_infos.head = new_player_info;
    player_infos.length++;

    pthread_mutex_unlock(&player_info_array_mutex);

    // printf("done\n");
    return 0;
}

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
    sockfds[0] = player_infos.length;
    pthread_mutex_unlock(&player_info_array_mutex);
    return sockfds;
}

char *package_global_player_info(int except_socketfd)
{
    pthread_mutex_lock(&player_info_array_mutex);
    player_info *info = player_infos.head;
    int have_set_len = 0;
    char *delimiter = "@";
    char *global_player_info = malloc(QUERY_BUFFER_LEN);
    // int max_buffer_size = UNIT_BUFFER_SIZE * 2;

    if (global_player_info == NULL ||
        player_infos.length == 1 || player_infos.length == 0)
    {
        pthread_mutex_unlock(&player_info_array_mutex);
        free(global_player_info);
        return NULL;
    }
    while (info)
    {
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

int scan_and_delete_unavailable_player_info()
{
    pthread_mutex_lock(&player_info_array_mutex);
    player_info *current = player_infos.head;
    player_info *previous = NULL;
    player_info *tmp = NULL;
    bool delete_flag = false;
    while (current != NULL)
    {
        if (!current->available && current->message_count == 0)
        {
            printf("(debug)prepare to delete unavailable player info\n");
            if (previous == NULL)
                player_infos.head = current->next;
            else
                previous->next = current->next;

            free(current->id);
            free(current->name);
            epoll_ctl(g_recv_epoll_fd, EPOLL_CTL_DEL, current->socketfd, NULL);
            if (current->havent_send > 0)
                epoll_ctl(g_send_epoll_fd, EPOLL_CTL_DEL, current->socketfd, NULL);
            close(current->socketfd);
            pthread_mutex_destroy(current->msg_count_mutex);
            free(current->msg_count_mutex);

            delete_flag = true;
            player_infos.length--;
        }
        previous = current;
        tmp = current;
        current = current->next;
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