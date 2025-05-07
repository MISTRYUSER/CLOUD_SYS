#include "header.h"


// 定义额外的TLV类型，用于注册、登录和响应的子步骤
typedef enum {
    TLV_TYPE_SALT = 15,              // 接收盐值
    TLV_TYPE_REGISTER_CONFIRM = 16,  // 注册确认
    TLV_TYPE_LOGIN_CONFIRM = 17,     // 登录确认
    TLV_TYPE_RESPONSE = 18           // 通用响应
} Additional_TLV_TYPE;

// 发送TLV包
// 功能：将TLV包（类型、长度、值）序列化并通过套接字发送
// 返回：0（成功），-1（失败）
int send_tlv(int sock, int type, const char* value) {
    tlv_packet_t tlv;
    tlv.type = type;
    strncpy(tlv.value, value, CHAR_SIZE - 1);
    tlv.value[CHAR_SIZE - 1] = '\0';
    tlv.length = strlen(tlv.value);
    ssize_t sent = send(sock, &tlv.type, sizeof(tlv.type), 0);
    ERROR_CHECK(sent, -1, "发送类型失败");
    sent = send(sock, &tlv.length, sizeof(tlv.length), 0);
    ERROR_CHECK(sent, -1, "发送长度失败");
    sent = send(sock, tlv.value, tlv.length, 0);
    ERROR_CHECK(sent, -1, "发送值失败");
    return 0;
}

// 接收TLV包
// 功能：从套接字接收TLV包并反序列化
// 返回：接收的长度，0（连接关闭），-1（错误）
int recv_tlv(int sock, tlv_packet_t *tlv) {
    ssize_t received = recv(sock, &tlv->type, sizeof(tlv->type), 0);
    if (received == 0) return 0; // 连接关闭
    ERROR_CHECK(received, -1, "接收类型失败");
    received = recv(sock, &tlv->length, sizeof(tlv->length), 0);
    if (received == 0) return 0;
    ERROR_CHECK(received, -1, "接收长度失败");
    received = recv(sock, tlv->value, tlv->length, 0);
    if (received == 0) return 0;
    ERROR_CHECK(received, -1, "接收值失败");
    tlv->value[tlv->length] = '\0';
    return tlv->length;
}

// 主函数
// 功能：连接服务端，处理用户输入并与服务端交互
int main() {
    // 创建套接字
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    ERROR_CHECK(sock, -1, "创建套接字失败");
    // 设置服务器地址
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);
    // 连接服务器
    int connect_ret = connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr));
    ERROR_CHECK(connect_ret, -1, "连接失败");

    // 提示用户选择操作
    printf("请选择操作：1. 注册 2. 登录 3. 注销\n");
    int choice;
    scanf("%d", &choice);

    if (choice == 1) {
        // 注册流程
        char username[50], password[100];
        printf("请输入用户名：");
        scanf("%s", username);
        send_tlv(sock, TLV_TYPE_USERREGISTER, username);
        tlv_packet_t tlv;
        if (recv_tlv(sock, &tlv) <= 0) {
            printf("接收失败\n");
            close(sock);
            return 1;
        }
        if (tlv.type == TLV_TYPE_SALT) {
            char* full_salt = tlv.value;
            printf("请输入密码：");
            scanf("%s", password);
            struct crypt_data data;
            memset(&data, 0, sizeof(data));
            char* encrypted_password = crypt_r(password, full_salt, &data);
            if (!encrypted_password) {
                printf("加密失败\n");
                close(sock);
                return 1;
            }
            send_tlv(sock, TLV_TYPE_REGISTER_CONFIRM, encrypted_password);
            if (recv_tlv(sock, &tlv) <= 0) {
                printf("接收失败\n");
                close(sock);
                return 1;
            }
            printf("响应：%s\n", tlv.value);
        } else {
            printf("错误：%s\n", tlv.value);
        }
    } else if (choice == 2) {
        // 登录流程
        char username[50], password[100];
        printf("请输入用户名：");
        scanf("%s", username);
        send_tlv(sock, TLV_TYPE_USERLOGIN, username);
        tlv_packet_t tlv;
        if (recv_tlv(sock, &tlv) <= 0) {
            printf("接收失败\n");
            close(sock);
            return 1;
        }
        if (tlv.type == TLV_TYPE_SALT) {
            char* full_salt = tlv.value;
            printf("请输入密码：");
            scanf("%s", password);
            struct crypt_data data;
            memset(&data, 0, sizeof(data));
            char* computed_hash = crypt_r(password, full_salt, &data);
            if (!computed_hash) {
                printf("加密失败\n");
                close(sock);
                return 1;
            }
            send_tlv(sock, TLV_TYPE_LOGIN_CONFIRM, computed_hash);
            if (recv_tlv(sock, &tlv) <= 0) {
                printf("接收失败\n");
                close(sock);
                return 1;
            }
            printf("响应：%s\n", tlv.value);
        } else {
            printf("错误：%s\n", tlv.value);
        }
    } else if (choice == 3) {
        // 注销流程
        char username[50];
        printf("请输入用户名：");
        scanf("%s", username);
        send_tlv(sock, TLV_TYPE_USERCANCEL, username);
        tlv_packet_t tlv;
        if (recv_tlv(sock, &tlv) <= 0) {
            printf("接收失败\n");
            close(sock);
            return 1;
        }
        printf("响应：%s\n", tlv.value);
    } else {
        printf("无效的选择\n");
    }

    close(sock);
    return 0;
}


