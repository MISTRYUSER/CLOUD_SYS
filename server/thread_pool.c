#include "thread_pool.h"

int tid_arr_init(tid_arr_t *tid_arr, int worker_num) {
    tid_arr->arr = (pthread_t *)calloc(worker_num, sizeof(pthread_t));
    tid_arr->worker_num = worker_num;
    return 0;
}

int thread_init(thread_pool_t *thread_pool, int worker_num) {
    tid_arr_init(&thread_pool->tid_arr, worker_num);
    task_queue_init(&thread_pool->task_queue);
    pthread_mutex_init(&thread_pool->mutex, NULL);
    pthread_cond_init(&thread_pool->cond, NULL);
    thread_pool->exit_flag = KEEP;
    return 0;
}
