#include "header.h"
#include "thread_pool.h"



#define ip "127.0.0.1"
#define data_port "12345"


int long_solve_command(int netfd, tlv_packet_t *tlv_packet, MYSQL *mysql, client_state_t* state) {
    char response[RESPONSE_SIZE] = {0};
    int ret = SUCCESS;
    TLV_TYPE type;
    uint16_t len;
    char value[CHAR_SIZE];

    // 解包TLV
    tlv_unpack(tlv_packet, &type, &len, value);
    value[len] = '\0';
    printf("长命令处理: type = %d, len = %d, value = %s\n", type, len, value);

    // 创建新的TCP监听端口用于数据传输
    int client_data_sock = tcp_init(ip, data_port);
    if (client_data_sock == -1) {
        snprintf(response, RESPONSE_SIZE, "创建数据连接失败");
        send_tlv(netfd, TLV_TYPE_RESPONSE, response);
        return FAILURE;
    }

    // 根据命令类型处理
    switch (type) {
    case TLV_TYPE_GETS:
        {
            printf("用户名 = %s\n", current_username);
            printf("处理下载文件请求: %s\n", value);
            // 假设 value 是文件名
            char *filename = value;
            // 获取当前路径
            char path[PATH_MAX] = {0};
            if (get_current_path(mysql, path, PATH_MAX) != SUCCESS) {
                snprintf(response, RESPONSE_SIZE, "获取当前路径失败");
                send_tlv(client_data_sock, TLV_TYPE_RESPONSE, response);
                break;
            }
            // 构建用户路径
            char userpath[PATH_MAX] = "/home/xuewentao/project/test_dir/";
            // 调用 file_gets 函数
            ret = file_gets(mysql, current_username, filename, path, response, RESPONSE_SIZE, userpath);
            if (ret == SUCCESS) {
                // 发送文件内容
                send_tlv(client_data_sock, TLV_TYPE_GETS, response);
            } else {
                send_tlv(client_data_sock, TLV_TYPE_RESPONSE, response);
            }
            break;
        }
    case TLV_TYPE_PUTS:
        {
            printf("处理上传文件请求: %s\n", value);
            // 假设 value 是 "filename|file_content"
            char *filename = strtok(value, "|");
            char *file_content = strtok(NULL, "|");
            if (!filename || !file_content) {
                snprintf(response, RESPONSE_SIZE, "无效的上传请求");
                int send_ret = send_tlv(client_data_sock, TLV_TYPE_RESPONSE, response);
                printf("send_ret = %d\n", send_ret);
                break;
            }
            // 获取当前路径
            char path[PATH_MAX] = {0};
            if (get_current_path(mysql, path, PATH_MAX) != SUCCESS) {
                snprintf(response, RESPONSE_SIZE, "获取当前路径失败");
                int send_ret = send_tlv(client_data_sock, TLV_TYPE_RESPONSE, response);
                printf("send_ret = %d\n", send_ret);
                break;
            }
            // 调用 file_puts 函数
            ret = file_puts(mysql, current_username, filename, path, file_content, strlen(file_content), response, RESPONSE_SIZE);
            if (ret == SUCCESS) {
                send_tlv(client_data_sock, TLV_TYPE_PUTS, "文件上传成功");
            } else {
                send_tlv(client_data_sock, TLV_TYPE_RESPONSE, response);
            }
            break;
        }
    default:
        {
            printf("收到未知长命令类型: %d, 值: %s\n", type, value);
            snprintf(response, RESPONSE_SIZE, "无效命令");
            send_tlv(client_data_sock, TLV_TYPE_RESPONSE, response);
            break;
        }
    }

    // 关闭数据连接
    close(client_data_sock);
    return ret;
}