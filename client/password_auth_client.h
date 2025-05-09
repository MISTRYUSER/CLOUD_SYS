#ifndef __PASSWORD_AUTH_CLIENT__
#define __PASSWORD_AUTH_CLIENT__
#include "header.h"


// 定义额外的TLV类型，用于注册、登录和响应的子步骤
// typedef enum {
//     TLV_TYPE_SALT = 15,              // 接收盐值
//     TLV_TYPE_REGISTER_CONFIRM = 16,  // 注册确认
//     TLV_TYPE_LOGIN_CONFIRM = 17,     // 登录确认
//     TLV_TYPE_RESPONSE = 18           // 通用响应
// } Additional_TLV_TYPE;

// 发送TLV包
// 功能：将TLV包（类型、长度、值）序列化并通过套接字发送
// 返回：0（成功），-1（失败）
int send_tlv(int sock, int type, const char* value);

// 接收TLV包
// 功能：从套接字接收TLV包并反序列化
// 返回：接收的长度，0（连接关闭），-1（错误）
int recv_tlv(int sock, tlv_packet_t *tlv);

void handle_register(int sock);

void handle_login(int sock);
     
void handle_cancel(int sock);

void handle_filesystem_commands(int sock,char *username);
#endif // __PASSWORD_AUTH_CLIENT__
