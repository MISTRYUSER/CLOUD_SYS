#include "thread_pool.h"
#include <time.h>
#define SOCKFD_SIZE 1024
#define MAX_CLIENTS 100 // 支持的最大客户端数量
#define TIMEOUT_SECONDS 30 // 超时时间，30秒

// 客户端状态结构体
typedef struct {
    int netfd;          // 客户端文件描述符
    char username[50];  // 用户名
    int logged_in;      // 是否已登录，0未登录，1已登录
    int current_pwd_id; // 当前目录ID
    char virtual_path[PATH_MAX]; // 当前虚拟路径
    int is_alive;       // 0 不活跃 1 活跃
    time_t last_active; // 上次活跃时间
} client_state_t;

// 全局客户端状态数组
client_state_t client_states[MAX_CLIENTS] = {0};
int client_count = 0;
__thread int current_pwd_id = 0;

int exit_pipe[2];
int sockfds[SOCKFD_SIZE] = {0};
char home[PATH_MAX] = "/home/jellison/cloud-drive/NetCloud";

MYSQL *mysql;
#define ip "192.168.108.130"
#define port "12345"

void handle(int signum) {
    printf("parent got signal = %d.\n", signum);
    write(exit_pipe[WRITE], "1", 1);
}

// 根据netfd查找或创建客户端状态
client_state_t* get_client_state(int netfd) {
    // 先查找现有客户端
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (client_states[i].is_alive && client_states[i].netfd == netfd) {
            return &client_states[i];
        }
    }
    // 查找空闲槽位
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (!client_states[i].is_alive) {
            client_states[i].netfd = netfd;
            client_states[i].logged_in = 0;
            client_states[i].current_pwd_id = 0;
            client_states[i].is_alive = 1;
            client_states[i].last_active = time(NULL); // 初始化活跃时间
            memset(client_states[i].username, 0, sizeof(client_states[i].username));
            memset(client_states[i].virtual_path, 0, sizeof(client_states[i].virtual_path));
            client_count++;
            return &client_states[i];
        }
    }
    // 如果没有空闲槽位，拒绝连接
    return NULL; // 客户端数量超出限制
}

// 删除客户端状态
void remove_client_state(int netfd) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (client_states[i].is_alive && client_states[i].netfd == netfd) {
            client_states[i].is_alive = 0;
            client_states[i].logged_in = 0;
            client_count--;
            break;
        }
    }
}

// 检查并处理超时客户端
void check_timeout_clients(int epfd) {
    time_t current_time = time(NULL);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (client_states[i].is_alive && (current_time - client_states[i].last_active) > TIMEOUT_SECONDS) {
            int netfd = client_states[i].netfd;
            printf("客户端超时，关闭连接，netfd = %d，用户名 = %s\n", netfd, client_states[i].username);
            send(netfd, "closed", 6, 0);
            epoll_del(epfd, netfd);
            close(netfd);
            client_states[i].is_alive = 0;
            remove_client_state(netfd);
        }
    }
}

int main(int argc, char *argv[]) {
    pipe(exit_pipe);
    if (fork()) {
        close(exit_pipe[READ]);
        signal(SIGUSR1, handle);
        wait(NULL);
        printf("parent exit.\n");
        exit(0);
    }
    close(exit_pipe[WRITE]);
    thread_pool_t thread_pool;
    thread_init(&thread_pool, WORKER_NUM);
    make_worker(&thread_pool);

    // connect MYSQL
    mysql = mysql_init(NULL);
    int db_connect = db_init(mysql);
    if (db_connect == 0) {
        printf("数据库连接成功\n");
    } else {
        printf("数据库连接失败\n");
        return 0;
    }

    int sockfd_index = 0;
    sockfds[sockfd_index] = tcp_init(ip, port);
    printf("sockfd = %d\n", sockfds[sockfd_index]);
    ERROR_CHECK(sockfds[sockfd_index], -1, "socket");
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        client_states[i].is_alive = 0;
    }
    client_count = 0;

    int epfd = epoll_create(1);
    epoll_add(epfd, sockfds[sockfd_index]);
    epoll_add(epfd, exit_pipe[READ]);
    time_t last_check_time = time(NULL);
    struct epoll_event ready_set[MAX_CLIENTS + 2]; // 足够容纳所有客户端和监听套接字
    while (1) {
        int ready_num = epoll_wait(epfd, ready_set, MAX_CLIENTS + 2, 1000); // 每1秒检查一次
        time_t current_time = time(NULL);

        // 每1秒检查一次超时客户端
        if (current_time - last_check_time >= 1) {
            check_timeout_clients(epfd);
            last_check_time = current_time;
        }

        for (int i = 0; i < ready_num; ++i) {
            int fd = ready_set[i].data.fd;
            if (fd == exit_pipe[READ]) {
                printf("thread pool exit\n");
                pthread_mutex_lock(&thread_pool.mutex);
                thread_pool.exit_flag = OUT;
                pthread_cond_broadcast(&thread_pool.cond);
                pthread_mutex_unlock(&thread_pool.mutex);
                for (int j = 0; j < WORKER_NUM; ++j) {
                    pthread_join(thread_pool.tid_arr.arr[j], NULL);
                }
                goto cleanup;
            } else if (fd == sockfds[sockfd_index]) {
                // 新客户端连接
                int netfd = accept(sockfds[sockfd_index], NULL, NULL);
                if (netfd == -1) {
                    perror("accept failed");
                    continue;
                }
                printf("新客户端连接，netfd = %d\n", netfd);
                client_state_t* state = get_client_state(netfd);
                if (state == NULL) {
                    printf("客户端数量超出限制，拒绝连接 netfd=%d\n", netfd);
                    close(netfd);
                    continue;
                }
                state->last_active = current_time; // 更新新连接的活跃时间
                epoll_add(epfd, netfd); // 将新客户端加入epoll监控
            } else {
                // 客户端数据可读
                int netfd = fd;
                client_state_t* state = get_client_state(netfd);
                if (state == NULL) {
                    printf("未找到客户端状态，关闭连接 netfd=%d\n", netfd);
                    epoll_del(epfd, netfd);
                    close(netfd);
                    continue;
                }
                state->last_active = current_time; // 更新活跃时间
                tlv_packet_t tlv_packet;
                bzero(&tlv_packet, sizeof(tlv_packet));
                int recv_result = recv_tlv(netfd, &tlv_packet);
                if (recv_result <= 0) {
                    printf("连接关闭，netfd = %d\n", netfd);
                    epoll_del(epfd, netfd);
                    remove_client_state(netfd);
                    close(netfd);
                    continue;
                }
                TLV_TYPE type = tlv_packet.type;
                if (type >= TLV_TYPE_CD && type <= TLV_TYPE_USERCANCEL) {
                    // 短命令在主线程处理
                    solve_command(netfd, &tlv_packet, mysql);
                } else if (type == TLV_TYPE_PUTS || type == TLV_TYPE_GETS) {
                    // 长命令加入线程池任务队列
                    long_command_t long_command_pack;
                    long_command_pack.netfd = netfd;
                    memcpy(&long_command_pack.tlv_packet, &tlv_packet, sizeof(tlv_packet));
                    pthread_mutex_lock(&thread_pool.mutex);
                    printf("长命令加入队列，netfd = %d, type = %d\n", netfd, type);
                    en_queue(&thread_pool.task_queue, long_command_pack);
                    pthread_cond_broadcast(&thread_pool.cond);
                    pthread_mutex_unlock(&thread_pool.mutex);
                } else {
                    printf("未知命令类型: %d\n", type);
                    char response[RESPONSE_SIZE] = "无效命令";
                    send_tlv(netfd, TLV_TYPE_RESPONSE, response);
                }
            }
        }
    }

cleanup:
    db_close(mysql);
    return 0;
}
