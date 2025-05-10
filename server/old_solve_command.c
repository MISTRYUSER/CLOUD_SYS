#include "header.h"
#include "thread_pool.h"

int solve_command(int netfd, tlv_packet_t *tlv_packet, MYSQL *mysql) {
    char response[RESPONSE_SIZE] = {0};
    int ret = SUCCESS;
    TLV_TYPE type;
    uint16_t len;
    char value[CHAR_SIZE];

    // 解包TLV
    tlv_unpack(tlv_packet, &type, &len, value);
    value[len] = '\0';
    if (type == 0 && len == 0) return 0;
    //printf("type = %d, len = %d, value = %s\n", type, len, value);
    static char current_username[50] = {0}; // 用于保存当前处理的用户名
    switch (type) {
    case TLV_TYPE_USERLOGIN: 
        {
            // 登录逻辑分为两个阶段：1. 发送用户名，2. 验证加密密码
            // 为每个用户维护独立的登录状态
            static struct {
                char username[50];
                int state; // 0表示等待用户名，1表示等待加密密码
            } 
            login_sessions[10] = {0}; // 支持最多10个并发登录会话
            static int session_count = 0;

            memset(current_username, 0, sizeof(current_username));

            // 查找当前连接的会话状态
            int session_index = -1;
            int is_new_session = 1;

            // 检查是否已有该连接的会话
            for (int i = 0; i < session_count; i++) {
                if (login_sessions[i].state == 1) { // 如果有等待密码的会话
                    session_index = i;
                    is_new_session = 0;
                    break;
                }
            }

            if (is_new_session) { // 新会话，等待用户名
                                  // 阶段1：接收用户名，查询并发送盐值
                strncpy(current_username, value, strlen(value));
                current_username[strlen(value)] = '\0';
                char salt[SALT_LEN + 1];
                if (get_salt(mysql, current_username, salt) == SUCCESS) {
                    // 创建新会话
                    if (session_count < 10) { // 确保不超过最大会话数
                        session_index = session_count++;
                        strncpy(login_sessions[session_index].username, current_username, sizeof(login_sessions[session_index].username) - 1);
                        login_sessions[session_index].state = 1; // 设置为等待密码状态

                        char full_salt[SALT_LEN + 20]; // 增加缓冲区大小以容纳前缀
                        snprintf(full_salt, sizeof(full_salt), "$6$%s", salt);
                        ret = send_tlv(netfd, TLV_TYPE_USERLOGIN, full_salt);
                        if (ret != 0) {
                            snprintf(response, RESPONSE_SIZE, "发送盐值失败");
                            send_tlv(netfd, TLV_TYPE_RESPONSE, response);
                            login_sessions[session_index].state = 0; // 重置状态
                            session_count--; // 移除会话
                        }
                    } else {
                        snprintf(response, RESPONSE_SIZE, "服务器会话已满，请稍后再试");
                        send_tlv(netfd, TLV_TYPE_RESPONSE, response);
                    }
                } else {
                    snprintf(response, RESPONSE_SIZE, "用户不存在");      
                    send_tlv(netfd, TLV_TYPE_RESPONSE, response);
                }
            } else { // 已有会话，等待密码
                     // 阶段2：接收加密密码并验证
                     // 从会话中获取用户名
                strncpy(current_username, login_sessions[session_index].username, sizeof(current_username) - 1);
                current_username[sizeof(current_username) - 1] = '\0';

                char stored_hash[128];
                if (get_encrypted_password(mysql, current_username, stored_hash) == SUCCESS) {
                    if (strcmp(value, stored_hash) == 0) {
                        snprintf(response, RESPONSE_SIZE, "登录成功");
                        send_tlv(netfd, TLV_TYPE_USERLOGIN, response);
                        current_pwd_id = 0; // 初始化为根目录
                        stack_init(); // 初始化目录栈
                    } else {
                        snprintf(response, RESPONSE_SIZE, "登录失败：密码错误");
                        send_tlv(netfd, TLV_TYPE_RESPONSE, response);
                    }
                } else {
                    snprintf(response, RESPONSE_SIZE, "用户不存在");
                    send_tlv(netfd, TLV_TYPE_RESPONSE, response);
                }

                // 重置会话状态
                login_sessions[session_index].state = 0;
                // 移动最后一个会话到当前位置，减少会话计数
                if (session_index < session_count - 1) {
                    login_sessions[session_index] = login_sessions[session_count - 1];
                }
                session_count--;
            }
            break;
        }
    case TLV_TYPE_USERREGISTER:     
        {
            // 注册逻辑分为两个阶段：1. 接收用户名，2. 接收加密密码并保存
            // 为每个用户维护独立的注册状态
            static struct {
                char username[50];
                char salt[25]; // 临时存储生成的盐值
                int state; // 0表示等待用户名，1表示等待加密密码
            }
            register_sessions[10] = {0}; // 支持最多10个并发注册会话
            static int reg_session_count = 0;
            printf("注册会话数: %d\n", reg_session_count);
            // 查找当前连接的注册会话状态
            int reg_session_index = -1;
            int is_new_reg_session = 1;

            // 检查是否已有该连接的注册会话
            for (int i = 0; i < reg_session_count; i++) {
                if (register_sessions[i].state == 1) { // 如果有等待密码的会话
                    reg_session_index = i;
                    is_new_reg_session = 0;
                    break;
                }
            }

            if (is_new_reg_session) { // 新会话，等待用户名
                                      // 阶段1：接收用户名，检查是否已存在并发送盐值
                memset(current_username, 0, sizeof(current_username));
                strncpy(current_username, value, strlen(value));
                current_username[strlen(value)] = '\0';

                if (user_exists(mysql, current_username)) {
                    snprintf(response, RESPONSE_SIZE, "用户名已存在");
                    send_tlv(netfd, TLV_TYPE_RESPONSE, response);
                } else {
                    // 创建新注册会话
                    if (reg_session_count < 10) { // 确保不超过最大会话数
                        reg_session_index = reg_session_count++;
                        strncpy(register_sessions[reg_session_index].username, current_username, sizeof(register_sessions[reg_session_index].username) - 1);
                        register_sessions[reg_session_index].state = 1; // 设置为等待密码状态

                        // 生成并保存盐值
                        memset(register_sessions[reg_session_index].salt, 0, sizeof(register_sessions[reg_session_index].salt));
                        printf("生成盐值...\n");
                        if (generate_salt(register_sessions[reg_session_index].salt, sizeof(register_sessions[reg_session_index].salt)) == SUCCESS) {
                            char full_salt[40];
                            snprintf(full_salt, sizeof(full_salt), "$6$%s", register_sessions[reg_session_index].salt);
                            ret = send_tlv(netfd, TLV_TYPE_USERREGISTER, full_salt);
                            if (ret != 0) {
                                snprintf(response, RESPONSE_SIZE, "发送盐值失败");
                                send_tlv(netfd, TLV_TYPE_RESPONSE, response);
                                register_sessions[reg_session_index].state = 0; // 重置状态
                                reg_session_count--; // 移除会话
                            }
                        } else {
                            snprintf(response, RESPONSE_SIZE, "生成盐值失败");
                            send_tlv(netfd, TLV_TYPE_RESPONSE, response);
                            register_sessions[reg_session_index].state = 0; // 重置状态
                            reg_session_count--; // 移除会话
                        }
                    } else {
                        snprintf(response, RESPONSE_SIZE, "服务器会话已满，请稍后再试");
                        send_tlv(netfd, TLV_TYPE_RESPONSE, response);
                    }
                }
            } else { // 已有会话，等待密码
                     // 阶段2：接收加密密码并保存到数据库
                     // 从会话中获取用户名和盐值
                strncpy(current_username, register_sessions[reg_session_index].username, sizeof(current_username) - 1);
                current_username[sizeof(current_username) - 1] = '\0';

                if (strlen(register_sessions[reg_session_index].salt) == 0) { // 检查临时盐值是否有效
                    snprintf(response, RESPONSE_SIZE, "无法获取盐值");
                    send_tlv(netfd, TLV_TYPE_RESPONSE, response);
                } else {
                    if (insert_user(mysql, current_username, register_sessions[reg_session_index].salt, value) == SUCCESS) {
                        snprintf(response, RESPONSE_SIZE, "注册成功");
                        send_tlv(netfd, TLV_TYPE_USERREGISTER, response);
                    } else {
                        snprintf(response, RESPONSE_SIZE, "注册失败：数据库错误");
                        send_tlv(netfd, TLV_TYPE_RESPONSE, response);
                    }
                }

                // 重置会话状态
                register_sessions[reg_session_index].state = 0;
                // 移动最后一个会话到当前位置，减少会话计数
                if (reg_session_index < reg_session_count - 1) {
                    register_sessions[reg_session_index] = register_sessions[reg_session_count - 1];
                }
                reg_session_count--;
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
            printf("进入了ls\n");
            ret = dir_ls(mysql, response ,RESPONSE_SIZE, current_username);
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
                send_tlv(netfd, TLV_TYPE_PWD, response);
            } else {
                send_tlv(netfd, TLV_TYPE_RESPONSE, response);
            }
            break;
        }
    case TLV_TYPE_MKDIR:
        {
            ret = dir_mkdir(mysql, current_username, value, response, RESPONSE_SIZE);
            if (ret == SUCCESS) {
                send_tlv(netfd, TLV_TYPE_MKDIR, response);
            } else {
                send_tlv(netfd, TLV_TYPE_RESPONSE, response);
            }
            break;
        }
    case TLV_TYPE_RMDIR:
        {
            ret = dir_rmdir(mysql, current_username, value, response, RESPONSE_SIZE);
            if (ret == SUCCESS) {
                send_tlv(netfd, TLV_TYPE_RMDIR, response);
            } else {
                send_tlv(netfd, TLV_TYPE_RESPONSE, response);
            }
            break;
        }
    default: 
        {
            printf("收到未知命令类型: %d, 值: %s\n", type, value);
            snprintf(response, RESPONSE_SIZE, "无效命令");
            send_tlv(netfd, TLV_TYPE_RESPONSE, response);
            break;
        }
    }
    return ret;
}

