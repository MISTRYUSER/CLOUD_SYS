#include "header.h"
#include "password_auth_client.h"
#include <crypt.h>
#define client_ip "127.0.0.1"
#define long_port "12345"
#define data_port "12345"
int sock; // 全局socket，避免线程间传递
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; // 互斥锁保护exit_num
// 监听服务器消息的线程函数
void* listen_server(void* arg) {
    char buf[RESPONSE_SIZE];
    while (1) {
        int len = recv(sock, buf, RESPONSE_SIZE, 0); // 阻塞式接收
        if (len > 0) {
            buf[len] = '\0'; // 确保字符串终止
            if (strcmp(buf, "closed") == 0) {
                printf("您已被强制下线\n");
                pthread_mutex_lock(&mutex);
                // exit_num = 0; // 设置退出标志
                pthread_mutex_unlock(&mutex);
                close(sock);
                exit(0);
            }
        } else if (len == 0) { // 连接关闭
            printf("服务器关闭连接\n");
            pthread_mutex_lock(&mutex);
            // exit_num = 0;
            pthread_mutex_unlock(&mutex);
            close(sock);
            exit(0);
        } else { // 接收错误
            perror("recv");
            break;
        }
    }
    return NULL;
}
int create_tcp_connection(const char *ip, const char *port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("socket");
        return -1;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(port));
    if (inet_pton(AF_INET, ip, &server_addr.sin_addr) <= 0) {
        perror("inet_pton");
        close(sock);
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("connect");
        close(sock);
        return -1;
    }

    return sock;
}
void handle_filesystem_commands(int sock ,char *username) {
    char command[1024];
    char virtual_path[PATH_MAX] = "/"; // 初始目录为根目录
    tlv_packet_t tlv;
    char value[4096] = {0};

    while (1) { 
        printf("[%s]请输入命令：\n", username);
        printf("pwd ls cd mkdir rmdir puts gets remove quit\n");
        // 添加非阻塞检查服务器消息
        char buf[RESPONSE_SIZE];
        int len = recv(sock, buf, RESPONSE_SIZE, MSG_DONTWAIT);
        if (scanf("%s", command) != 1) {
            fprintf(stderr, "输入错误\n");
            continue;
        }

        if (strcmp(command, "pwd") == 0) {
            printf("当前目录：%s\n", virtual_path);
        } else if (strcmp(command, "ls") == 0) {
            // 发送LS命令请求
            int ret = send_tlv(sock, TLV_TYPE_LS, "");
            printf("发送ls命令: %d\n", ret);
            
            // 接收服务器响应
            memset(&tlv, 0, sizeof(tlv));
            printf("准备接收服务器响应...\n");
            int recv_ret = recv_tlv(sock, &tlv);
            printf("接收结果: %d, type: %d, length: %d\n", recv_ret, tlv.type, tlv.length);
            
            if (recv_ret >= 0) {
                if (tlv.type == TLV_TYPE_LS) {
                    printf("目录内容：\n%s\n", tlv.value);
                } else if (tlv.type == TLV_TYPE_RESPONSE) {
                    printf("服务器错误: %s\n", tlv.value);
                } else {
                    printf("未知响应类型: %d\n", tlv.type);
                }
            } else {
                printf("接收服务器响应失败，错误码: %d\n", recv_ret);
            }
        }  else if (strcmp(command, "cd") == 0) {
        char path[1024];
        printf("请输入目录路径：");
        scanf("%s", path);
        send_tlv(sock, TLV_TYPE_CD, path);
        if (recv_tlv(sock, &tlv) > 0 && tlv.type == TLV_TYPE_CD) {
            // 直接使用服务端返回的路径
            strncpy(virtual_path, tlv.value, PATH_MAX - 1);
            virtual_path[PATH_MAX - 1] = '\0';
            printf("目录切换成功：%s\n", virtual_path);
        } else {
            printf("接收目录切换响应失败\n");
        }
    } else if (strcmp(command, "mkdir") == 0) {
            char dirname[1024];
            printf("请输入目录名：");
            scanf("%s", dirname);
            send_tlv(sock, TLV_TYPE_MKDIR, dirname);
            if (recv_tlv(sock, &tlv) > 0 && tlv.type == TLV_TYPE_MKDIR) {
                printf("响应：%s\n", tlv.value);
            } else {
                printf("接收响应失败\n");
            }
        } else if (strcmp(command, "rmdir") == 0) {
            char rmdirname[1024];
            printf("请输入目录名：");
            scanf("%s", rmdirname);
            send_tlv(sock, TLV_TYPE_RMDIR, rmdirname);
            if (recv_tlv(sock, &tlv) > 0 && tlv.type == TLV_TYPE_RMDIR) {
                printf("响应：%s\n", tlv.value);
            } else {
                printf("接收响应失败\n");
            }
        } else if (strcmp(command, "puts") == 0) {
        char filename[1024];
        char file_path[PATH_MAX];
        printf("请输入要上传的文件名：");
        scanf("%s", filename);
        printf("请输入本地文件路径：");
        scanf("%s", file_path);
        FILE *fp = fopen(file_path, "rb");
        if (fp == NULL) {
            printf("无法打开文件: %s\n", file_path);
            continue;
        }
        fseek(fp, 0, SEEK_END);
        long file_size = ftell(fp);
        fseek(fp, 0, SEEK_SET);
        char *file_content = malloc(file_size + 1);
        fread(file_content, 1, file_size, fp);
        file_content[file_size] = '\0';
        fclose(fp);

        // 创建数据传输连接
        int data_sock = create_tcp_connection(client_ip, long_port);
        printf("data_sock = %d\n", data_sock);
        if (data_sock == -1) {
            printf("创建数据连接失败\n");
            free(file_content);
            continue;
        }

        // 发送文件名和文件内容
        snprintf(value, sizeof(value), "%s|%s", filename, file_content);
        free(file_content);
        send_tlv(data_sock, TLV_TYPE_PUTS, value);
        int recv_id = recv_tlv(data_sock, &tlv);
        printf("recv_id = %d\n", recv_id);
        // 接收服务器响应
        if (recv_id >= 0 ) {
            printf("服务器响应: %s\n", tlv.value);
        } else {
            printf("接收服务器响应失败\n");
        }
        // close(data_sock);
    } else if (strcmp(command, "gets") == 0) {
    char filename[1024];
    printf("请输入要下载的文件名：");
    scanf("%s", filename);

    // 创建数据传输连接
    int data_sock = create_tcp_connection(client_ip, long_port);
    printf("data_sock = %d\n", data_sock);
    if (data_sock == -1) {
        printf("创建数据连接失败\n");
        continue;
    }

    // 发送文件名
    send_tlv(data_sock, TLV_TYPE_GETS, filename);

    // 接收文件内容
    recv_tlv(data_sock, &tlv);
   // if ( tlv.type == TLV_TYPE_GETS) {
        char local_file_path[PATH_MAX] = {0};
snprintf(local_file_path, sizeof(local_file_path),
         "/home/xuewentao/project/test_dir/%s/%s", username, filename);
        FILE *fp = fopen(local_file_path, "wb");
        if (fp) {
            fwrite(tlv.value, 1, tlv.length, fp);
            fclose(fp);
            printf("文件下载成功，保存到: %s\n", local_file_path);
        } else {
            printf("无法保存文件: %s\n", local_file_path);
        }
   // } 
    // else {
    //     printf("文件下载失败: %s\n", tlv.value);
    // }

    close(data_sock);
}else if (strcmp(command, "remove") == 0) {
            char filename[1024];
            printf("请输入要删除的文件名：");
            scanf("%s", filename);
            send_tlv(sock, TLV_TYPE_REMOVE, filename);
            if (recv_tlv(sock, &tlv) >= 0 && tlv.type == TLV_TYPE_REMOVE) {
                printf("文件删除成功: %s\n", tlv.value);
            } else {
                printf("文件删除失败: %s\n", tlv.value);
            }
        } else if (strcmp(command, "quit") == 0) {
            close(sock);
            exit(0);
        }
        else {
            printf("无效命令\n");
        }
        // exit_num = 0;
//        printf("ff\n");
    }//while(exit_num)
    printf("您已被强制下线\n");

    exit(0);

}

void handle_login(int sock) {
    char username[50], password[100];
    printf("请输入用户名：");
    scanf("%s", username);
    send_tlv(sock, TLV_TYPE_USERLOGIN, username);
    printf("type = %d\n", TLV_TYPE_USERLOGIN);
    tlv_packet_t tlv;
    if (recv_tlv(sock, &tlv) <= 0) {
        printf("接收失败\n");
        close(sock);
        return;
    }

    if (tlv.type == TLV_TYPE_USERLOGIN) {
        printf("请输入密码：");
        scanf("%s", password);
        struct crypt_data data = {0};
        char* encrypted = crypt_r(password, tlv.value, &data);
        if (!encrypted) {
            printf("加密失败\n");
            close(sock);
            return;
        }
        send_tlv(sock, TLV_TYPE_USERLOGIN, encrypted);
        if (recv_tlv(sock, &tlv) > 0 && tlv.type == TLV_TYPE_USERLOGIN && strcmp(tlv.value, "登录成功") == 0) {
            printf("登录成功\n");
            handle_filesystem_commands(sock,username); // 调用文件系统命令处理
        } else {
            printf("登录失败：%s\n", tlv.value);
        }
    } else {
        printf("错误响应类型：%d\n", tlv.type);
    }
}

void handle_register(int sock) {
    char username[50], password[100];
    printf("请输入用户名：");
    scanf("%s", username);
    send_tlv(sock, TLV_TYPE_USERREGISTER, username);

    tlv_packet_t tlv;
    if (recv_tlv(sock, &tlv) <= 0) {
        printf("接收失败\n");
        close(sock);
        return;
    }
    printf("type = %d, len = %d, value = %s\n", tlv.type, tlv.length, tlv.value);
    if (tlv.type == TLV_TYPE_USERREGISTER) {
        printf("请输入密码：");
        scanf("%s", password);
        struct crypt_data data = {0};
        char* encrypted = crypt_r(password, tlv.value, &data);
        if (!encrypted) {
            printf("加密失败\n");
            close(sock);
            return;
        }
        send_tlv(sock, TLV_TYPE_USERREGISTER, encrypted);
        recv_tlv(sock, &tlv);
        printf("响应：%s\n", tlv.value);
    } else {
        printf("错误响应类型：%d\n", tlv.type);
    }
}

void handle_cancel(int sock) {
    char username[50];
    printf("请输入用户名：");
    scanf("%s", username);
    send_tlv(sock, TLV_TYPE_USERCANCEL, username);

    tlv_packet_t tlv;
    if (recv_tlv(sock, &tlv) <= 0) {
        printf("接收失败\n");
        close(sock);
        return;
    }

    printf("响应：%s\n", tlv.value);
}
