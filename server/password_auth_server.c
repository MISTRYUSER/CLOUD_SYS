#include "header.h"

// 生成随机盐值
int generate_salt(char* salt,size_t salt_size) {
    static const char alphanum[] = "./0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    size_t alphanum_size = sizeof(alphanum);
    if (salt_size < 21) {
        printf("生成盐值失败: 缓冲区不足\n");
        return FAILURE;
    }
    for (int i = 0; i < 20; i++) {
        size_t index = (double)rand() / RAND_MAX * (sizeof(alphanum) - 1);
        salt[i] = alphanum[index];
    }
    salt[20] = '\0';
    printf("生成盐值成功: %s\n", salt);
    return SUCCESS;
}

// 检查用户是否存在
int user_exists(MYSQL *mysql, const char* username) {
    if (!mysql || !username) {
        printf("无效的 mysql 指针或用户名\n");
        return -1;
    }

    if (mysql_ping(mysql) != 0) {
        printf("数据库连接已断开: %s\n", mysql_error(mysql));
        return -1;
    }
    printf("连接状态正常\n");

    char escaped_username[512];
    size_t username_len = strlen(username);
    if (username_len * 2 + 1 > sizeof(escaped_username)) {
        printf("缓冲区不足以转义用户名\n");
        return -1;
    }

    mysql_real_escape_string(mysql, escaped_username, username, username_len);
    char query[1024];
    snprintf(query, sizeof(query), "SELECT COUNT(*) FROM user_info WHERE username = '%s' AND is_deleted = 0", escaped_username);

    MYSQL_RES *result;
    if (db_query(mysql, query, &result) != SUCCESS) {
        printf("查询用户存在性失败\n");
        return -1;
    }

    MYSQL_ROW row = mysql_fetch_row(result);
    int count = row ? atoi(row[0]) : 0;
    mysql_free_result(result);
    printf("用户 %s 存在性检查: %s\n", username, count > 0 ? "存在" : "不存在");
    return count > 0;
}

// 插入新用户
int insert_user(MYSQL *mysql, const char* username, const char* salt, const char* encrypted_password) {
    char escaped_username[512];
    mysql_real_escape_string(mysql, escaped_username, username, strlen(username));
    char escaped_salt[512];
    mysql_real_escape_string(mysql, escaped_salt, salt, strlen(salt));
    char escaped_encrypted_password[512];
    mysql_real_escape_string(mysql, escaped_encrypted_password, encrypted_password, strlen(encrypted_password));
    char query[2048];
    snprintf(query, sizeof(query), "INSERT INTO user_info (username, salt, encrypted_password, is_deleted) VALUES ('%s', '%s', '%s', 0)", escaped_username, escaped_salt, escaped_encrypted_password);

    my_ulonglong affected_rows;
    if (db_query(mysql, query, &affected_rows) != SUCCESS) {
        printf("用户 %s 注册失败\n", username);
        return FAILURE;
    }
    if (affected_rows > 0) {
        printf("用户 %s 注册成功\n", username);
        return SUCCESS;
    } else {
        printf("用户 %s 注册失败: 没有插入行\n", username);
        return FAILURE;
    }
}

// 获取用户盐值
int get_salt(MYSQL *mysql, const char* username, char* salt) {
    char escaped_username[512];
    mysql_real_escape_string(mysql, escaped_username, username, strlen(username));
    char query[1024];
    snprintf(query, sizeof(query), "SELECT salt FROM user_info WHERE username = '%s' AND is_deleted = 0", escaped_username);

    MYSQL_RES *result;
    if (db_query(mysql, query, &result) != SUCCESS) {
        printf("获取盐值失败\n");
        return FAILURE;
    }

    MYSQL_ROW row = mysql_fetch_row(result);
    if (row && row[0]) {
        strncpy(salt, row[0], 20);
        salt[20] = '\0';
        printf("获取盐值成功: %s\n", salt);
        mysql_free_result(result);
        return SUCCESS;
    }
    mysql_free_result(result);
    return FAILURE;
}

// 获取用户加密密码
int get_encrypted_password(MYSQL *mysql, const char* username, char* encrypted_password) {
    char escaped_username[512];
    mysql_real_escape_string(mysql, escaped_username, username, strlen(username));
    char query[1024];
    snprintf(query, sizeof(query), "SELECT encrypted_password FROM user_info WHERE username = '%s' AND is_deleted = 0", escaped_username);

    MYSQL_RES *result;
    if (db_query(mysql, query, &result) != SUCCESS) {
        printf("获取加密密码失败\n");
        return FAILURE;
    }

    MYSQL_ROW row = mysql_fetch_row(result);
    if (row && row[0]) {
        strncpy(encrypted_password, row[0], 128);
        encrypted_password[128] = '\0';
        printf("获取加密密码成功: %s\n", encrypted_password);
        mysql_free_result(result);
        return SUCCESS;
    }
    mysql_free_result(result);
    return FAILURE;
}
//物理删除
int delete_user(MYSQL *mysql, const char* username) {
    char escaped_username[512];
    mysql_real_escape_string(mysql, escaped_username, username, strlen(username));
    char query[1024];
    snprintf(query, sizeof(query), "DELETE FROM user_info WHERE username = '%s'", escaped_username);

    my_ulonglong affected_rows;
    if (db_query(mysql, query, &affected_rows) != SUCCESS) {
        printf("用户 %s 注销失败\n", username);
        return FAILURE;
    }
    if (affected_rows > 0) {
        printf("用户 %s 注销成功\n", username);
        return SUCCESS;
    } else {
        printf("用户 %s 注销失败: 没有找到该用户\n", username);
        return FAILURE;
    }
}

// 发送TLV包
int send_tlv(int sock, int type, const char* value) {
    tlv_packet_t tlv;
    memset(&tlv, 0, sizeof(tlv_packet_t));
    tlv.type = (TLV_TYPE)type;
    strncpy(tlv.value, value, CHAR_SIZE - 1);
    tlv.value[CHAR_SIZE - 1] = '\0';
    tlv.length = strlen(tlv.value);
    
    ssize_t sent;
    size_t total = 0;
    while (total < sizeof(tlv.type)) {
        sent = send(sock, (char*)&tlv.type + total, sizeof(tlv.type) - total, 0);
//        ERROR_CHECK(sent, -1, "发送类型失败");
        total += sent;
    }
    total = 0;
    while (total < sizeof(tlv.length)) {
        sent = send(sock, (char*)&tlv.length + total, sizeof(tlv.length) - total, 0);
//        ERROR_CHECK(sent, -1, "发送长度失败");
        total += sent;
    }
    total = 0;
    while (total < tlv.length) {
        sent = send(sock, tlv.value + total, tlv.length - total, 0);
//        ERROR_CHECK(sent, -1, "发送值失败");
        total += sent;
    }
    printf("发送成功 - 类型: %d, 值: %s\n", type, value);
    return 0;
}

// 接收TLV包
int recv_tlv(int sock, tlv_packet_t *tlv) {
    memset(tlv, 0, sizeof(tlv_packet_t));
    ssize_t received;
    size_t total = 0;
    char buffer[sizeof(tlv->type) + sizeof(tlv->length) + CHAR_SIZE];
    memset(buffer, 0, sizeof(buffer));

    while (total < sizeof(tlv->type)) {
        received = recv(sock, buffer + total, sizeof(tlv->type) - total, 0);
        if (received <= 0) {
            if (received == 0) printf("1连接关闭\n");
//            else ERROR_CHECK(received, -1, "接收类型失败");
            return received;
        }
        total += received;
    }
    memcpy(&tlv->type, buffer, sizeof(tlv->type));
    printf("接收到 type: %d\n", tlv->type);
    total = 0;

    while (total < sizeof(tlv->length)) {
        received = recv(sock, buffer + total, sizeof(tlv->length) - total, 0);
        if (received <= 0) {
            if (received == 0) printf("2连接关闭\n");
            else ERROR_CHECK(received, -1, "接收长度失败");
            return received;
        }
        total += received;
    }
    memcpy(&tlv->length, buffer, sizeof(tlv->length));
    printf("接收到 length: %u\n", tlv->length);
    total = 0;

    if (tlv->length > CHAR_SIZE - 1) {
        printf("长度超限, 丢弃包\n");
        return -1;
    }
    while (total < tlv->length) {
        received = recv(sock, buffer + total, tlv->length - total, 0);
        if (received <= 0) {
            if (received == 0) printf("3连接关闭\n");
//            else ERROR_CHECK(received, -1, "接收值失败");
            return received;
        }
        total += received;
    }
    memcpy(tlv->value, buffer, tlv->length);
    tlv->value[tlv->length] = '\0';
    printf("TLV 包接收成功 - 类型: %d, 值: %s\n", tlv->type, tlv->value);
    return tlv->length;
}
