#include "header.h"
#include "password_auth_client.h"

void handle_filesystem_commands(int sock ,char *username) {
    char command[1024];
    char virtual_path[PATH_MAX] = "/"; // 初始目录为根目录

    while (1) {
        printf("[%s]请输入命令：\n",username);
        printf("pwd ls cd mkdir rmdir puts gets quit");
        scanf("%s", command);

        if (strcmp(command, "pwd") == 0) {
            printf("当前目录：%s\n", virtual_path);
        } else if (strcmp(command, "ls") == 0) {
            send_tlv(sock, TLV_TYPE_LS, "");
            tlv_packet_t tlv;
            if (recv_tlv(sock, &tlv) > 0 && tlv.type == TLV_TYPE_LS) {
                printf("目录内容：%s\n", tlv.value);
            } else {
                printf("tlv中内容：%s\n", tlv.value);
                printf("tlv中 type:%d\n", tlv.type);
                printf("接收目录内容失败\n");
            }
        } else if (strcmp(command, "cd") == 0) {
            char path[1024];
            printf("请输入目录路径：");
            scanf("%s", path);
            send_tlv(sock, TLV_TYPE_CD, path);
            tlv_packet_t tlv;
            if (recv_tlv(sock, &tlv) > 0 && tlv.type == TLV_TYPE_CD) {
                printf("tlv中内容：%s\n", tlv.value);
                    // 更新 virtual_path
                    if (path[0] == '/') {
                        strncpy(virtual_path, path, PATH_MAX - 1);
                    } else {
                        strcat(virtual_path, "/");
                        strncat(virtual_path, path, PATH_MAX - strlen(virtual_path) - 1);
                    }
                    printf("目录切换成功：%s\n", virtual_path);
            } else {
                printf("接收目录切换响应失败\n");
            }
        } else if (strcmp(command, "mkdir") == 0) {
            char dirname[1024];
            printf("请输入目录名：");
            scanf("%s", dirname);
            send_tlv(sock, TLV_TYPE_MKDIR, dirname);
            tlv_packet_t tlv;
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
            tlv_packet_t tlv;
            if (recv_tlv(sock, &tlv) > 0 && tlv.type == TLV_TYPE_RMDIR) {
                printf("响应：%s\n", tlv.value);
            } else {
                printf("接收响应失败\n");
            }
        } else if (strcmp(command, "puts") == 0) {
            printf("文件上传功能待实现\n");
        } else if (strcmp(command, "gets") == 0) {
            printf("文件下载功能待实现\n");
        } else if (strcmp(command, "quit") == 0) {
            close(sock);
            exit(0);
        } else {
            printf("无效命令\n");
        }
    }
}

void handle_login(int sock) {
    char username[50], password[100];
    printf("请输入用户名：");
    scanf("%s", username);
    send_tlv(sock, TLV_TYPE_USERLOGIN, username);

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