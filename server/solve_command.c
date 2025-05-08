#include "header.h"
#include "thread_pool.h"
#include "config.h"
#include "password_auth_server.h"

int solve_command(int netfd, tlv_packet_t *tlv_packet, MYSQL *mysql) {
    char response[RESPONSE_SIZE] = {0};
    int ret = SUCCESS;
    TLV_TYPE type;
    uint16_t len;
    char value[CHAR_SIZE];

    // 解包TLV
    tlv_unpack(tlv_packet, &type, &len, value);
    value[len] = '\0';

    switch (type) {
    case TLV_TYPE_USERLOGIN: 
        {
            // 登录逻辑分为两个阶段：1. 发送用户名，2. 验证加密密码
            static char current_username[50] = {0}; // 用于保存当前处理的用户名
            static int login_state = 0; // 0表示等待用户名，1表示等待加密密码

            if (login_state == 0) {
                // 阶段1：接收用户名，查询并发送盐值
                strncpy(current_username, value, strlen(value));
                current_username[strlen(value)] = '\0';
                char salt[SALT_LEN + 1];
                if (get_salt(mysql, current_username, salt) == SUCCESS) {
                    char full_salt[SALT_LEN + 1];
                    snprintf(full_salt, sizeof(full_salt), "$6$%s", salt);
                    ret = send_tlv(netfd, TLV_TYPE_SALT, full_salt);
                    if (ret == 0) {
                        login_state = 1; // 进入下一阶段：等待加密密码
                    } else {
                        snprintf(response, RESPONSE_SIZE, "发送盐值失败");
                        send_tlv(netfd, TLV_TYPE_RESPONSE, response);
                    }
                } else {
                    snprintf(response, RESPONSE_SIZE, "用户不存在");      
                    send_tlv(netfd, TLV_TYPE_RESPONSE, response);
                    login_state = 0; // 重置状态
                }
            } else if (login_state == 1) {
                // 阶段2：接收加密密码并验证
                char stored_hash[128];
                if (get_encrypted_password(mysql, current_username, stored_hash) == SUCCESS) {
                    if (strcmp(value, stored_hash) == 0) {
                        snprintf(response, RESPONSE_SIZE, "登录成功");
                        send_tlv(netfd, TLV_TYPE_USERLOGIN, response);
                    } else {
                        snprintf(response, RESPONSE_SIZE, "登录失败：密码错误");
                        send_tlv(netfd, TLV_TYPE_RESPONSE, response);
                    }
                } else {
                    snprintf(response, RESPONSE_SIZE, "用户不存在");
                    send_tlv(netfd, TLV_TYPE_RESPONSE, response);
                }
                login_state = 0; // 重置状态，处理下一个登录请求
                memset(current_username, 0, sizeof(current_username));
            }
            break;
        }
    case TLV_TYPE_USERREGISTER:     
        {
            // 注册逻辑分为两个阶段：1. 接收用户名，2. 接收加密密码并保存
            static char current_username[50] = {0}; // 用于保存当前处理的用户名
            static int register_state = 0; // 0表示等待用户名，1表示等待加密密码

            if (register_state == 0) {
                // 阶段1：接收用户名，检查是否已存在并发送盐值
                 strncpy(current_username, value, strlen(value));
                current_username[strlen(value)] = '\0';

                if (user_exists(mysql, current_username)) {
                    snprintf(response, RESPONSE_SIZE, "用户名已存在");
                    send_tlv(netfd, TLV_TYPE_RESPONSE, response);
                    register_state = 0; // 重置状态
                } else {
                    char salt[SALT_LEN + 1];
                    if (generate_salt(salt) == SUCCESS) {
                        char full_salt[40];
                        snprintf(full_salt, sizeof(full_salt), "$6$%s", salt);
                        ret = send_tlv(netfd, TLV_TYPE_SALT, full_salt);
                        if (ret == 0) {
                            register_state = 1; // 进入下一阶段：等待加密密码
                        } else {
                            snprintf(response, RESPONSE_SIZE, "发送盐值失败");
                            send_tlv(netfd, TLV_TYPE_RESPONSE, response);
                            register_state = 0; // 重置状态
                        }
                    } else {
                        snprintf(response, RESPONSE_SIZE, "生成盐值失败");
                        send_tlv(netfd, TLV_TYPE_RESPONSE, response);
                        register_state = 0; // 重置状态
                    }
                }
            } else if (register_state == 1) {
                // 阶段2：接收加密密码并保存到数据库
                char salt[SALT_LEN + 1];
                if (get_salt(mysql, current_username, salt) != SUCCESS) { // 临时保存盐值可能需要额外存储，这里假设可以重新生成或从上下文获取
                    snprintf(response, RESPONSE_SIZE, "无法获取盐值");
                    send_tlv(netfd, TLV_TYPE_RESPONSE, response);
                } else {
                    if (insert_user(mysql, current_username, salt, value) == SUCCESS) {
                        snprintf(response, RESPONSE_SIZE, "注册成功");
                        send_tlv(netfd, TLV_TYPE_USERREGISTER, response);
                    } else {
                        snprintf(response, RESPONSE_SIZE, "注册失败：数据库错误");
                        send_tlv(netfd, TLV_TYPE_RESPONSE, response);
                    }
                }
                register_state = 0; // 重置状态，处理下一个注册请求
                memset(current_username, 0, sizeof(current_username));
            }
            break;
        }
    case TLV_TYPE_USERCANCEL: 
        {
            // 用户注销逻辑：直接逻辑删除用户
            char username[50];
            strncpy(username, value, sizeof(username) - 1);
            username[sizeof(username) - 1] = '\0';

            if (delete_user(mysql, username) == SUCCESS) {
                snprintf(response, RESPONSE_SIZE, "注销成功");
                send_tlv(netfd, TLV_TYPE_USERCANCEL, response);
            } else {
                snprintf(response, RESPONSE_SIZE, "注销失败：用户不存在或数据库错误");
                send_tlv(netfd, TLV_TYPE_RESPONSE, response);
            }
            break;
        }
    case TLV_TYPE_LS: 
        {
            ret = dir_ls(mysql, value, response, RESPONSE_SIZE);
            if (ret == SUCCESS) {
                send_tlv(netfd, TLV_TYPE_LS, response);
            } else {
                send_tlv(netfd, TLV_TYPE_RESPONSE, response);
            }
            break;
        }
    case TLV_TYPE_CD:
        {
            ret = dir_cd(mysql, value, response, RESPONSE_SIZE);
            if (ret == SUCCESS) {
                send_tlv(netfd, TLV_TYPE_CD, response);
            } else {
                send_tlv(netfd, TLV_TYPE_RESPONSE, response);
            }
            break;
        }
    case TLV_TYPE_PWD:
        {
            ret = dir_pwd(mysql, response);
            if (ret == SUCCESS) {
                send_tlv(netfd, TLV_TYPE_CD, response);
            } else {
                send_tlv(netfd, TLV_TYPE_RESPONSE, response);
            }
            break;
        }
    case TLV_TYPE_MKDIR:
        {
            ret = dir_mkdir();
        }
    default: 
        {
            snprintf(response, RESPONSE_SIZE, "无效命令");
            send_tlv(netfd, TLV_TYPE_RESPONSE, response);
            break;
        }
    }
    return ret;
}

// 假设的 tlv_unpack 函数，用于从 tlv_packet_t 中提取 type, length, value
void tlv_unpack(tlv_packet_t *tlv_packet, TLV_TYPE *type, uint16_t *len, char *value) {
    *type = tlv_packet->type;
    *len = tlv_packet->length;
    strncpy(value, tlv_packet->value, tlv_packet->length);
    value[tlv_packet->length] = '\0';
}
