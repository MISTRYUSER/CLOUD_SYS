#include "header.h"

// 定义额外的TLV类型，用于注册、登录和响应的子步骤
typedef enum {
    TLV_TYPE_SALT = 15,              // 发送盐值
    TLV_TYPE_REGISTER_CONFIRM = 16,  // 注册确认
    TLV_TYPE_LOGIN_CONFIRM = 17,     // 登录确认
    //TLV_TYPE_RESPONSE = 18           // 通用响应
} Additional_TLV_TYPE;

// 初始化数据库连接
// 功能：初始化 MySQL 连接
// 参数：mysql - MySQL连接句柄
// 返回：SUCCESS（0）成功，ERR_DB（-3）失败
int db_init(MYSQL *mysql) {
    mysql = mysql_init(NULL);
    if (!mysql) return ERR_DB;
    if (!mysql_real_connect(mysql, "localhost", "root", "", "my_database", 0, NULL, 0)) {
        mysql_close(mysql);
        return ERR_DB;
    }
    return SUCCESS;
}

// 关闭数据库连接
// 功能：释放 MySQL 连接资源
// 参数：mysql - MySQL连接句柄
// 返回：SUCCESS（0）成功，ERR_DB（-3）失败
int db_close(MYSQL *mysql) {
    if (mysql) mysql_close(mysql);
    return SUCCESS;
}

// 生成随机盐值
// 功能：生成 20 字符的随机盐值，用于 SHA512 加密
// 返回：SUCCESS（0）成功，FAILURE（-1）失败
int generate_salt(char* salt) {
    static const char alphanum[] = "./0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    if (strlen(salt) < 21) return FAILURE; // 20 字符 + 空终止符
    for (int i = 0; i < 20; i++) {
        size_t index = (double)rand() / RAND_MAX * (sizeof(alphanum) - 1);
        salt[i] = alphanum[index];
    }
    salt[20] = '\0';
    return SUCCESS;
}

// 检查用户是否存在
// 功能：查询数据库，检查用户名是否已存在且未被逻辑删除
// 返回：1（存在），0（不存在），-1（错误）
int user_exists(MYSQL *mysql, const char* username) {
    char escaped_username[512];
    mysql_real_escape_string(mysql, escaped_username, username, strlen(username));
    char query[1024];
    snprintf(query, sizeof(query), "SELECT COUNT(*) FROM user_info WHERE username = '%s' AND is_deleted = 0", escaped_username);
    int ret = mysql_query(mysql, query);
    if (ret != 0) {
        return -1; // 错误
    }
    MYSQL_RES* result = mysql_store_result(mysql);
    if (result) {
        MYSQL_ROW row = mysql_fetch_row(result);
        int count = atoi(row[0]);
        mysql_free_result(result);
        return count > 0;
    }
    return -1; // 错误
}

// 插入新用户
// 功能：将用户名、盐值和加密密码存储到数据库
// 返回：SUCCESS（0）成功，FAILURE（-1）失败
int insert_user(MYSQL *mysql, const char* username, const char* salt, const char* encrypted_password) {
    char escaped_username[512];
    mysql_real_escape_string(mysql, escaped_username, username, strlen(username));
    char escaped_salt[512];
    mysql_real_escape_string(mysql, escaped_salt, salt, strlen(salt));
    char escaped_encrypted_password[512];
    mysql_real_escape_string(mysql, escaped_encrypted_password, encrypted_password, strlen(encrypted_password));
    char query[2048]; // 增加查询缓冲区大小以避免截断
    snprintf(query, sizeof(query), "INSERT INTO user_info (username, salt, encrypted_password, is_deleted) VALUES ('%s', '%s', '%s', 0)", escaped_username, escaped_salt, escaped_encrypted_password);
    int ret = mysql_query(mysql, query);
    return ret == 0 ? SUCCESS : FAILURE;
}

// 获取用户盐值
// 功能：从数据库检索指定用户的盐值
// 返回：SUCCESS（0）成功，FAILURE（-1）失败
int get_salt(MYSQL *mysql, const char* username, char* salt) {
    char escaped_username[512];
    mysql_real_escape_string(mysql, escaped_username, username, strlen(username));
    char query[1024];
    snprintf(query, sizeof(query), "SELECT salt FROM user_info WHERE username = '%s' AND is_deleted = 0", escaped_username);
    int ret = mysql_query(mysql, query);
    if (ret != 0) {
        return FAILURE;
    }
    MYSQL_RES* result = mysql_store_result(mysql);
    if (result) {
        MYSQL_ROW row = mysql_fetch_row(result);
        if (row && row[0]) {
            strncpy(salt, row[0], 20); // 假设盐值为20字符
            salt[20] = '\0';
            mysql_free_result(result);
            return SUCCESS;
        }
        mysql_free_result(result);
    }
    return FAILURE;
}

// 获取用户加密密码
// 功能：从数据库检索指定用户的加密密码
// 返回：SUCCESS（0）成功，FAILURE（-1）失败
int get_encrypted_password(MYSQL *mysql, const char* username, char* encrypted_password) {
    char escaped_username[512];
    mysql_real_escape_string(mysql, escaped_username, username, strlen(username));
    char query[1024];
    snprintf(query, sizeof(query), "SELECT encrypted_password FROM user_info WHERE username = '%s' AND is_deleted = 0", escaped_username);
    int ret = mysql_query(mysql, query);
    if (ret != 0) {
        return FAILURE;
    }
    MYSQL_RES* result = mysql_store_result(mysql);
    if (result) {
        MYSQL_ROW row = mysql_fetch_row(result);
        if (row && row[0]) {
            strncpy(encrypted_password, row[0], 128); // 假设最大长度
            encrypted_password[128] = '\0';
            mysql_free_result(result);
            return SUCCESS;
        }
        mysql_free_result(result);
    }
    return FAILURE;
}

// 逻辑删除用户
// 功能：将指定用户的 is_deleted 字段设置为 1
// 返回：SUCCESS（0）成功，FAILURE（-1）失败
int delete_user(MYSQL *mysql, const char* username) {
    char escaped_username[512];
    mysql_real_escape_string(mysql, escaped_username, username, strlen(username));
    char query[1024];
    snprintf(query, sizeof(query), "UPDATE user_info SET is_deleted = 1 WHERE username = '%s'", escaped_username);
    int ret = mysql_query(mysql, query);
    return ret == 0 ? SUCCESS : FAILURE;
}

// 发送TLV包
// 功能：将TLV包（类型、长度、值）序列化并通过套接字发送
// 返回：0（成功），-1（失败）
int send_tlv(int sock, TLV_TYPE type, const char* value) {
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

// 处理客户端请求
// 功能：处理单个客户端连接的TLV包序列
// 参数：arg - 客户端套接字文件描述符
void* handle_client(void* arg) {
    int client_fd = *(int*)arg;
    free(arg);

    // 为每个线程创建独立的数据库连接
    MYSQL mysql;
    if (db_init(&mysql) != SUCCESS) {
        close(client_fd);
        return NULL;
    }

    tlv_packet_t tlv;
    if (recv_tlv(client_fd, &tlv) <= 0) {
        db_close(&mysql);
        close(client_fd);
        return NULL;
    }

    if (tlv.type == TLV_TYPE_USERREGISTER) {
        char username[50];
        sscanf(tlv.value, "%s", username);
        if (user_exists(&mysql, username)) {
            send_tlv(client_fd, TLV_TYPE_RESPONSE, "用户名已存在");
        } else {
            char salt[21];
            generate_salt(salt);
            char full_salt[40];
            snprintf(full_salt, sizeof(full_salt), "$6$%s", salt);
            send_tlv(client_fd, TLV_TYPE_USERREGISTER, full_salt);
            if (recv_tlv(client_fd, &tlv) <= 0) {
                db_close(&mysql);
                close(client_fd);
                return NULL;
            }
            if (tlv.type == TLV_TYPE_REGISTER_CONFIRM) {
                char* encrypted_password = tlv.value;
                if (insert_user(&mysql, username, salt, encrypted_password) == SUCCESS) {
                    send_tlv(client_fd, TLV_TYPE_RESPONSE, "注册成功");
                } else {
                    send_tlv(client_fd, TLV_TYPE_RESPONSE, "注册失败");
                }
            } else {
                send_tlv(client_fd, TLV_TYPE_RESPONSE, "无效序列");
            }
        }
    } else if (tlv.type == TLV_TYPE_USERLOGIN) {
        char username[50];
        sscanf(tlv.value, "%s", username);
        char salt[21];
        if (get_salt(&mysql, username, salt) == SUCCESS) {
            char full_salt[40];
            snprintf(full_salt, sizeof(full_salt), "$6$%s", salt);
            send_tlv(client_fd, TLV_TYPE_USERREGISTER, full_salt);
            if (recv_tlv(client_fd, &tlv) <= 0) {
                db_close(&mysql);
                close(client_fd);
                return NULL;
            }
            if (tlv.type == TLV_TYPE_LOGIN_CONFIRM) {
                char* received_hash = tlv.value;
                char stored_hash[128];
                if (get_encrypted_password(&mysql, username, stored_hash) == SUCCESS) {
                    if (strcmp(received_hash, stored_hash) == 0) {
                        send_tlv(client_fd, TLV_TYPE_RESPONSE, "登录成功");
                    } else {
                        send_tlv(client_fd, TLV_TYPE_RESPONSE, "登录失败");
                    }
                } else {
                    send_tlv(client_fd, TLV_TYPE_RESPONSE, "用户不存在");
                }
            } else {
                send_tlv(client_fd, TLV_TYPE_RESPONSE, "无效序列");
            }
        } else {
            send_tlv(client_fd, TLV_TYPE_RESPONSE, "用户不存在");
        }
    } else if (tlv.type == TLV_TYPE_USERCANCEL) {
        char username[50];
        sscanf(tlv.value, "%s", username);
        if (delete_user(&mysql, username) == SUCCESS) {
            send_tlv(client_fd, TLV_TYPE_RESPONSE, "注销成功");
        } else {
            send_tlv(client_fd, TLV_TYPE_RESPONSE, "注销失败");
        }
    } else {
        send_tlv(client_fd, TLV_TYPE_RESPONSE, "无效命令");
    }

    db_close(&mysql);
    close(client_fd);
    return NULL;
}

// 主函数
// 功能：创建服务端套接字，监听并为每个客户端连接创建线程
int main() {
    srand(time(NULL)); // 初始化随机种子

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    ERROR_CHECK(server_fd, -1, "创建套接字失败");
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);
    int bind_ret = bind(server_fd, (struct sockaddr*)&address, sizeof(address));
    ERROR_CHECK(bind_ret, -1, "绑定失败");
    int listen_ret = listen(server_fd, 10);
    ERROR_CHECK(listen_ret, -1, "监听失败");

    while (1) {
        int client_fd = accept(server_fd, NULL, NULL);
        ERROR_CHECK(client_fd, -1, "接受连接失败");
        pthread_t thread_id;
        int* client_fd_ptr = (int *)malloc(sizeof(int));
        *client_fd_ptr = client_fd;
        pthread_create(&thread_id, NULL, handle_client, client_fd_ptr);
        pthread_detach(thread_id);
    }

    close(server_fd);
    return 0;
}



