#include "header.h"
#include "password_auth_client.h"


// 功能：连接服务端，处理用户输入并与服务端交互
int main() {
    int  exit_num = 1;
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) { perror("创建套接字失败"); return -1; }

    struct sockaddr_in server_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(12345)
    };
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("连接失败");
        return -1;
    }

    int epfd = epoll_create(1);
    epoll_add(epfd, sock);
    epoll_add(epfd, STDIN_FILENO);
    struct epoll_event ready_set[1024]; // 足够容纳所有客户端和监听套接字

    int choice;
    while (exit_num) {
        printf("请选择操作：1. 注册 2. 登录 3. 注销 0. 退出\n");
        char buf[RESPONSE_SIZE];
        //还是无法退出
        // 使用epoll_wait等待事件
        int ready_num = epoll_wait(epfd, ready_set, 1024, -1); // -1表示阻塞等待
        if (ready_num == -1) {
            perror("epoll_wait失败");
            continue;
        }

        for (int i = 0; i < ready_num; ++i) {
            int fd = ready_set[i].data.fd;
            if (fd == sock) {
                recv(sock, buf, RESPONSE_SIZE, 0);
                if(strcmp(buf,"closed") == 0) {
                    printf("您已被强制下线\n");
                    epoll_del(epfd, sock);
                    exit_num = 0;

                    exit(0);
                }
            }else if(exit_num == 1 && fd == STDIN_FILENO) {
                scanf("%d", &choice);
                switch (choice) {
                case 1: handle_register(sock); break;
                case 2: handle_login(sock); break;
                case 3: handle_cancel(sock); break;
                case 0: close(sock); return 0;
                }
            }
        }
    }
}

