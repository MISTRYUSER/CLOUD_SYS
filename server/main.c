#include "thread_pool.h"
__thread char current_user[50] = {0};
__thread char local_path[PATH_MAX] = {0};
__thread char virtual_path[PATH_MAX] = {0};
__thread int current_pwd_id = 0;
int exit_pipe[2];
#define ip "127.0.0.1" 
#define port "12345"
void handle(int  signum) {
    printf("parent got signal = %d.\n", signum);
    write(exit_pipe[WRITE], "1", 1);
}
/* ./ftp_server 192.168.108.130 12345 */
int main(int argc, char *argv[]) {
    // ARGS_CHECK(argc, 3);
    pipe(exit_pipe);
    // 父进程
    if (fork()) {
        close(exit_pipe[READ]);
        signal(SIGUSR1, handle);
        wait(NULL);
        printf("parent exit.\n");
        exit(0);
    }
    // 子进程
    close(exit_pipe[WRITE]);
    thread_pool_t thread_pool;
    thread_init(&thread_pool, WORKER_NUM);
    make_worker(&thread_pool);

    int sockfd = tcp_init(ip, port);
    ERROR_CHECK(sockfd, -1, "socket");
    int epfd = epoll_create(1);
    epoll_add(epfd, sockfd);
    epoll_add(epfd, exit_pipe[READ]);
    struct epoll_event ready_set[1024];
    while(1) {
        int ready_num = epoll_wait(epfd, ready_set, 1024, -1);
        for (int i = 0; i < ready_num; ++i) {
            int fd = ready_set[i].data.fd;
            if (fd == sockfd) {
                int netfd = accept(sockfd, NULL, NULL);
                pthread_mutex_lock(&thread_pool.mutex);
                printf("master, netfd = %d\n", netfd);
                en_queue(&thread_pool.task_queue, netfd);
                pthread_cond_broadcast(&thread_pool.cond);
                pthread_mutex_unlock(&thread_pool.mutex);
            } else if (fd == exit_pipe[READ]) {
                printf("thread pool exit\n");
                pthread_mutex_lock(&thread_pool.mutex);
                thread_pool.exit_flag = OUT;
                pthread_cond_broadcast(&thread_pool.cond);
                pthread_mutex_unlock(&thread_pool.mutex);

                for (int j = 0; j < WORKER_NUM; ++j) {
                    pthread_join(thread_pool.tid_arr.arr[j], NULL);
                }
            }
        }
    }
    return 0;
}
