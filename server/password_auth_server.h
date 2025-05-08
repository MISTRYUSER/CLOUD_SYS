#ifndef __PASSWORD_AUTH_SERVER__
#define __PASSWORD_AUTH_SERVER__

#include "header.h"

// 初始化数据库连接
// 功能：初始化 MySQL 连接
// 参数：mysql - MySQL连接句柄
// 返回：SUCCESS（0）成功，ERR_DB（-3）失败
int db_init(MYSQL *mysql);

// 关闭数据库连接
// 功能：释放 MySQL 连接资源
// 参数：mysql - MySQL连接句柄
// 返回：SUCCESS（0）成功，ERR_DB（-3）失败
int db_close(MYSQL *mysql);

// 生成随机盐值
// 功能：生成 20 字符的随机盐值，用于 SHA512 加密
// 返回：SUCCESS（0）成功，FAILURE（-1）失败
int generate_salt(char* salt);

// 检查用户是否存在
// 功能：查询数据库，检查用户名是否已存在且未被逻辑删除
// 返回：1（存在），0（不存在），-1（错误）
int user_exists(MYSQL *mysql, const char* username);

// 插入新用户
// 功能：将用户名、盐值和加密密码存储到数据库
// 返回：SUCCESS（0）成功，FAILURE（-1）失败
int insert_user(MYSQL *mysql, const char* username, const char* salt, const char* encrypted_password);

// 获取用户盐值
// 功能：从数据库检索指定用户的盐值
// 返回：SUCCESS（0）成功，FAILURE（-1）失败
int get_salt(MYSQL *mysql, const char* username, char* salt);

// 获取用户加密密码
// 功能：从数据库检索指定用户的加密密码
// 返回：SUCCESS（0）成功，FAILURE（-1）失败
int get_encrypted_password(MYSQL *mysql, const char* username, char* encrypted_password);

// 逻辑删除用户
// 功能：将指定用户的 is_deleted 字段设置为 1
// 返回：SUCCESS（0）成功，FAILURE（-1）失败
int delete_user(MYSQL *mysql, const char* username);

// 发送TLV包
// 功能：将TLV包（类型、长度、值）序列化并通过套接字发送
// 返回：0（成功），-1（失败）
int send_tlv(int sock, TLV_TYPE type, const char* value);

// 接收TLV包
// 功能：从套接字接收TLV包并反序列化
// 返回：接收的长度，0（连接关闭），-1（错误）
int recv_tlv(int sock, tlv_packet_t *tlv);

// 处理客户端请求
// 功能：处理单个客户端连接的TLV包序列
// 参数：arg - 客户端套接字文件描述符
void* handle_client(void* arg);

#endif // __PASSWORD_AUTH_SERVER__
