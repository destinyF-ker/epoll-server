#include "query_list.h"

pthread_mutex_t g_free_list_mutex;
pthread_mutex_t g_ready_list_mutex;
pthread_mutex_t g_work_list_mutex;

int init_query_list_lock()
{
    pthread_mutex_init(&g_free_list_mutex, NULL);
    pthread_mutex_init(&g_ready_list_mutex, NULL);
    pthread_mutex_init(&g_work_list_mutex, NULL);
    return 0;
}

int destroy_query_list_lock()
{
    pthread_mutex_destroy(&g_free_list_mutex);
    pthread_mutex_destroy(&g_ready_list_mutex);
    pthread_mutex_destroy(&g_work_list_mutex);
    return 0;
}

CQuery *get_free_query()
{
    CQuery *pQuery = NULL;
    pthread_mutex_lock(&g_free_list_mutex);
    if (NULL == g_pfree_list)
    {
        pthread_mutex_unlock(&g_free_list_mutex);
        return NULL;
    }
    pQuery = g_pfree_list;

    g_pfree_list = CQuery_get_next_query(g_pfree_list);
    if (NULL != g_pfree_list)
        CQuery_set_pre_query(g_pfree_list, NULL);
    pthread_mutex_unlock(&g_free_list_mutex);

    CQuery_set_pre_query(pQuery, NULL);
    CQuery_set_next_query(pQuery, NULL);
    return pQuery;
}

int add_free_list(CQuery *pQuery)
{
    pthread_mutex_lock(&g_free_list_mutex);
    CQuery_init(pQuery);
    if (NULL == g_pfree_list)
    {
        CQuery_set_pre_query(pQuery, NULL);
        CQuery_set_next_query(pQuery, NULL);
        g_pfree_list = pQuery;
    }
    else
    {
        CQuery_set_pre_query(pQuery, NULL);
        CQuery_set_next_query(pQuery, g_pfree_list);
        CQuery_set_pre_query(g_pfree_list, pQuery);
        g_pfree_list = pQuery;
    }
    pthread_mutex_unlock(&g_free_list_mutex);
    return 0;
}

//========================================

CQuery *get_gready_query()
{
    CQuery *pQuery = NULL;
    pthread_mutex_lock(&g_ready_list_mutex);
    if (NULL == g_pready_list)
    {
        pthread_mutex_unlock(&g_ready_list_mutex);
        return NULL;
    }
    pQuery = g_pready_list;

    g_pready_list = CQuery_get_next_query(g_pready_list);
    if (NULL != g_pready_list)
        CQuery_set_pre_query(g_pready_list, NULL);

    pthread_mutex_unlock(&g_ready_list_mutex);

    CQuery_set_pre_query(pQuery, NULL);
    CQuery_set_next_query(pQuery, NULL);

    return pQuery;
}

int add_gready_list(CQuery *pQuery)
{
    pthread_mutex_lock(&g_ready_list_mutex);
    if (NULL == g_pready_list)
    {
        CQuery_set_pre_query(pQuery, NULL);
        CQuery_set_next_query(pQuery, NULL);

        g_pready_list = pQuery;
        g_pready_list_tail = pQuery;
    }
    else
    {
        CQuery_set_pre_query(pQuery, g_pready_list_tail);
        CQuery_set_next_query(pQuery, NULL);
        CQuery_set_next_query(g_pready_list_tail, pQuery);

        g_pready_list_tail = pQuery;
    }
    pthread_mutex_unlock(&g_ready_list_mutex);
    return 0;
}

CQuery *get_gwork_query()
{
    CQuery *pQuery = NULL;
    pthread_mutex_lock(&g_work_list_mutex);
    if (NULL == g_pwork_list)
    {
        pthread_mutex_unlock(&g_work_list_mutex);
        return NULL;
    }
    pQuery = g_pwork_list;

    g_pwork_list = CQuery_get_next_query(g_pwork_list);
    if (NULL != g_pwork_list)
        CQuery_set_pre_query(g_pwork_list, NULL);
    pthread_mutex_unlock(&g_work_list_mutex);

    CQuery_set_pre_query(pQuery, NULL);
    CQuery_set_next_query(pQuery, NULL);
    return pQuery;
}

int add_gwork_list(CQuery *pQuery)
{
    pthread_mutex_lock(&g_work_list_mutex);
    if (NULL == g_pwork_list)
    {
        CQuery_set_pre_query(pQuery, NULL);
        CQuery_set_next_query(pQuery, NULL);

        g_pwork_list = pQuery;
        g_pwork_list_tail = pQuery;
    }
    else
    {
        CQuery_set_pre_query(pQuery, g_pwork_list_tail);
        CQuery_set_next_query(pQuery, NULL);
        CQuery_set_next_query(g_pwork_list_tail, pQuery);

        g_pwork_list_tail = pQuery;
    }
    pthread_mutex_unlock(&g_work_list_mutex);
    return 0;
}

int del_gwork_list(CQuery *pQuery)
{
    pthread_mutex_lock(&g_work_list_mutex);
    if (NULL != CQuery_get_pre_query(pQuery))
    {
        // pQuery->get_pre_query()->set_next_query(pQuery->get_next_query());
        CQuery *pre_query = CQuery_get_pre_query(pQuery);
        CQuery *next_query = CQuery_get_next_query(pQuery);
        CQuery_set_next_query(pre_query, next_query);
    }
    else
    {
        // g_pwork_list = pQuery->get_next_query();
        g_pwork_list = CQuery_get_next_query(pQuery);
    }
    if (NULL != CQuery_get_next_query(pQuery))
    {
        // pQuery->get_next_query()->set_pre_query(pQuery->get_pre_query());
        CQuery *next_query = CQuery_get_next_query(pQuery);
        CQuery *pre_query = CQuery_get_pre_query(pQuery);
        CQuery_set_pre_query(next_query, pre_query);
    }
    pthread_mutex_unlock(&g_work_list_mutex);
    CQuery_set_pre_query(pQuery, NULL);
    CQuery_set_next_query(pQuery, NULL);
    return 0;
}