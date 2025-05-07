<<<<<<< Updated upstream
#include "thread_pool.h"

int exit_pipe[2];

void handle(int  signum) {
    printf("parent got signal = %d.\n", signum);
    write(exit_pipe[WRITE], "1", 1);
}
/* ./ftp_server 192.168.108.130 12345 */
int main(int argc, char *argv[]) {
    ARGS_CHECK(argc, 3);
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

    int sockfd = tcp_init(argv[1], argv[2]);
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
=======
#include "header.h"
extern __thread int pwd = 0;//应该为 0
int main()
{
    MYSQL *mysql = mysql_init(NULL);
    // 初始化数据库连接
    int n = db_init(mysql);
    if (n == 0)
    {
        printf("数据库连接成功！\n");
    }
    else
    {
        printf("数据库连接失败！\n");
        return -1;
    }
    char result[1024] = {0};
    char sql[1024] = {0};
    snprintf(sql,sizeof(sql),"select * from file_info");
    db_query(mysql,sql,result);
    printf("result:%s\n",result);
    result[0] = '\0';
    dir_cd(mysql,"/home",result,1024);
    printf("result:%s\n",result);
    result[0] = '\0';
    int close = db_close(mysql); // 关闭数据库连接
    if(close == 0)
    {
        printf("数据库关闭成功！\n");
>>>>>>> Stashed changes
    }
    return 0;
}
