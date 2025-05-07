#include "header.h"

#define WORKER_NUM 5

#define WRITE   1
#define READ    0

#define OUT     1
#define KEEP    0

typedef struct tid_arr_s {      // 线程 ---- tid
    pthread_t *arr;             // tid数组的首地址
    int worker_num;             // 数组长度
} tid_arr_t;

typedef struct node_s {
    int netfd;
    struct node_s *next;
} node_t;

typedef struct task_queue_s {   // 任务队列 
    node_t *front;
    node_t *rear;
    int size;
} task_queue_t;

typedef struct thread_pool_s {
    tid_arr_t tid_arr;          // 工人信息
    task_queue_t task_queue;    // 任务队列
    pthread_mutex_t mutex;      // 互斥锁
    pthread_cond_t cond;        // 条件变量
    int exit_flag;              // 退出标志 0不退出，1退出
} thread_pool_t;

int task_queue_init(task_queue_t *queue);
int en_queue(task_queue_t *queue, int netfd);
int de_queue(task_queue_t *queue);
int print_queue(task_queue_t *queue);

int tid_arr_init(tid_arr_t *tid_arr, int worker_num);

int thread_init(thread_pool_t *thread_pool, int worker_num);
int make_worker(thread_pool_t *thread_pool);
int epoll_add(int epfd, int fd);
int epoll_del(int epfd, int fd);
int tcp_init(char *ip, char *port);

