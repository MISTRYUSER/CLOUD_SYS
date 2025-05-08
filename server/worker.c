#include "thread_pool.h"

void set_nonblock(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

void *thread_func(void *arg) {
    thread_pool_t *thread_pool = (thread_pool_t *)arg;
    int netfd;
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
        netfd = thread_pool->task_queue.front->netfd;
        printf("thread get netfd = %d\n", netfd);
        de_queue(&thread_pool->task_queue);
        pthread_mutex_unlock(&thread_pool->mutex);

        // connect MYSQL
        MYSQL *mysql = mysql_init(NULL);
        int db_connect = db_init(mysql);
        if (db_connect == 0) {
            printf("数据库连接成功\n");
        } else {
            printf("数据库连接失败\n");
            return NULL;
        }
        // solve command
        while (1) {
            // TODO tlv_packet free 在断开连接时候
            tlv_packet_t *tlv_packet = (tlv_packet_t *)malloc(TLV_SIZE);
            recv_tlv(netfd, tlv_packet);
            solve_command(netfd, tlv_packet, mysql);

        }
    }
}

int make_worker(thread_pool_t *thread_pool) {
    for (int i = 0; i < thread_pool->tid_arr.worker_num; ++i) {
        pthread_create(&thread_pool->tid_arr.arr[i], NULL, thread_func, thread_pool);
    }
}
