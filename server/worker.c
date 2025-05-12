#include "thread_pool.h"

extern MYSQL *mysql;

void set_nonblock(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}
void *thread_func(void *arg) {
    thread_pool_t *thread_pool = (thread_pool_t *)arg;
    while (1) {
        printf("thread working\n");
        pthread_mutex_lock(&thread_pool->mutex);
        while (thread_pool->task_queue.size == 0 && thread_pool->exit_flag == KEEP) {
            pthread_cond_wait(&thread_pool->cond, &thread_pool->mutex);
        }
        if (thread_pool->exit_flag == OUT) {
            printf("thread out\n");
            pthread_mutex_unlock(&thread_pool->mutex);
            pthread_exit(NULL);
        }
        // 直接使用任务中的 netfd，而不是调用 accept
        printf("子线程开始处理任务\n");
// 获取任务并从队列中移除
        int netfd = thread_pool->task_queue.front->long_command.netfd;
        tlv_packet_t long_tlv_packet = thread_pool->task_queue.front->long_command.tlv_packet;
        client_state_t* state = thread_pool->task_queue.front->long_command.state;
        de_queue(&thread_pool->task_queue);
        pthread_mutex_unlock(&thread_pool->mutex);
        long_solve_command(netfd, &long_tlv_packet, mysql, state);
        printf("命令处理结束\n");

        // 关闭连接
        close(netfd);
    }
    return NULL;
}

int make_worker(thread_pool_t *thread_pool) {
    for (int i = 0; i < thread_pool->tid_arr.worker_num; ++i) {
        pthread_create(&thread_pool->tid_arr.arr[i], NULL, thread_func, thread_pool);
    }
    return 0;
}
