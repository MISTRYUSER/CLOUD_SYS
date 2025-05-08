#ifndef THREAD_POOl
#define THREAD_POOL
#include "header.h"
#include "config.h"
#define WORKER_NUM 5
#define MAX_CLIENTS 10

#define WRITE   1
#define READ    0

#define OUT     1
#define KEEP    0

extern __thread char current_user[50]; // 线程本地存储的用户名

#define TLV_SIZE sizeof(tlv_packet_t)
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

// 登录状态结构（简化版，需根据实际线程模型扩展）
typedef struct {
    int step;            // 0: 未登录, 1: 已发送盐值
    char username[256];  // 当前登录的用户名
    char salt[SALT_LEN]; // 用户的盐值
} LoginState;


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

/**
 * @brief 通过用户名查询单个字符串字段（如 salt、password）
 * @param mysql          MySQL连接句柄
 * @param query_template SQL模板（需包含一个?占位符）
 * @param username       用户名
 * @param result         结果输出缓冲区
 * @param result_size    缓冲区大小
 * @return SUCCESS/ERR_PARAM/ERR_DB
 */
int fetch_string_by_username(MYSQL *mysql, const char *query_template, const char *username, char *result, size_t result_size);
int solve_command(int netfd, tlv_packet_t *tlv_packet, MYSQL *mysql);

int tlv_unpack(const tlv_packet_t *packet, TLV_TYPE *type, uint16_t *len, char *value);
int tlv_pack(tlv_packet_t *packet, TLV_TYPE type, uint16_t len, const char *value);

#endif
