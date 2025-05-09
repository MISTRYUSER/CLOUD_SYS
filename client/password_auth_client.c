#include "header.h"
#include "password_auth_client.h"


// 功能：连接服务端，处理用户输入并与服务端交互
int main() {
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
    
        int choice;
        while (1) {
            printf("请选择操作：1. 注册 2. 登录 3. 注销 0. 退出\n");
            scanf("%d", &choice);
            switch (choice) {
                case 1: handle_register(sock); break;
                case 2: handle_login(sock); break;
                case 3: handle_cancel(sock); break;
                case 0: close(sock); return 0;
                default: printf("无效选择\n");
            }
        }
    }


