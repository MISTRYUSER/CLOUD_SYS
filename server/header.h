#ifndef HEADER_H
#define HEADER_H
#include "my_libs.h"

extern char home[PATH_MAX];		// 网盘在服务端的根目录

// 为每个线程创建一个独立的副本
extern __thread char local_path[PATH_MAX];	 // 当前服务端的目录路径 
extern __thread char virtual_path[PATH_MAX]; // 当前客户端的目录路径

#define  UPDATE_LOCAL_PATH do {strcpy(local_path, home);strncat(local_path, virtual_path, PATH_MAX - strlen(home) -1);} while(0)

/*
 * 命令类型(type)定义
 */
typedef enum {
    TLV_TYPE_TRANSFILE = 1,  // 用来表示传输文件以及传输目录
    TLV_TYPE_CD,             // cd 命令：切换目录
    TLV_TYPE_LS,             // ls 命令：列出目录
    TLV_TYPE_PUTS,           // puts 命令：上传文件
    TLV_TYPE_GETS,           // gets 命令：下载文件
    TLV_TYPE_REMOVE,         // remove 命令：删除文件
    TLV_TYPE_PWD,            // pwd 命令：显示当前目录
    TLV_TYPE_MKDIR,          // mkdir 命令：新建目录
    TLV_TYPE_RMDIR,          // rmdir 命令：删除目录
    TLV_TYPE_USERINFO,       // 用户信息
    TLV_TYPE_FILEINFO        // 文件元信息
} TLV_TYPE;
//文件类型
typedef enum {
    FILE_TYPE_UNKNOWN = 0,   // 未知类型
    FILE_TYPE_REGULAR,       // 普通文件
    FILE_TYPE_DIRECTORY,     // 目录
    FILE_TYPE_SYMLINK,       // 符号链接
    FILE_TYPE_SOCKET,        // 套接字
    FILE_TYPE_CHAR_DEVICE,   // 字符设备
    FILE_TYPE_BLOCK_DEVICE,  // 块设备
    FILE_TYPE_FIFO           // 命名管道
} FILE_TYPE;
#define CHAR_SIZE 4096
#define RESPONSE_SIZE 4096
/*
 * TLV 协议数据包结构:
 * Type  : uint16_t 命令类型（如cd、ls、puts等）
 * Length: uint16_t Value字段长度
 * Value : 数据内容，长度为Length字节
 */
typedef struct {
    TLV_TYPE type;          // 命令类型
    uint16_t length;        // Value字段长度
    char value[CHAR_SIZE];	// 实际数据
} tlv_packet_t;

/**
 * @file cloud_storage.h
 * @brief 云存储项目第三期头文件
 *
 * 本头文件定义了云存储项目第三期所需的错误代码、常量、结构体、函数原型等。
 */

/**
 * @defgroup error_codes 错误代码
 * @{
 */
#define SUCCESS 0     /**< 操作成功 */
#define FAILURE -1    /**< 操作失败 */
#define ERR_PARAM -2  /**< 参数错误 */
#define ERR_DB -3     /**< 数据库错误 */
/** @} */

/**
 * @defgroup constants 常量
 * @{
 */
#define HASH_LEN 41   /**< 哈希值长度（SHA1: 40 + 1） */
#define SALT_LEN 20   /**< 盐值长度 */
/** @} */

/**
 * @defgroup structures 结构体
 * @{
 */

/**
 * @brief 用户信息结构体
 */
typedef struct {
    char username[50];           /**< 用户名 */
    char salt[SALT_LEN];         /**< 盐值 */
    char encrypted_password[HASH_LEN]; /**< 加密后的密码 */
} User;

/**
 * @brief 文件元数据结构体
 */
typedef struct {
    int id;                      /**< 文件ID */
    char filename[256];          /**< 文件名 */
    char username[50];           /**< 所有者用户名 */
    int parent_id;               /**< 父目录ID */
    char hash[HASH_LEN];         /**< 文件哈希值 */
    char path[1024];             /**< 文件路径 */
    FILE_TYPE type;              /**< 文件类型**/
    int is_directory;            /**< 是否为目录（0: 文件, 1: 目录） */
} FileMeta;
/** @} */

/**
 * @defgroup database_management 数据库管理
 * @defgroup database_management 数据库管理
 * @{
 */

/**
 * @brief 初始化数据库连接
 * @return SUCCESS 成功, ERR_DB 失败
 */
int db_init();

/**
 * @brief 关闭数据库连接
 * @return SUCCESS 成功, ERR_DB 失败
 */
int db_close();
/** @} */

/**
 * @defgroup user_management 用户管理
 * @{
 */

/**
 * @brief 用户注册
 * @param username 用户名
 * @param password 密码
 * @return SUCCESS 成功, FAILURE 失败, ERR_PARAM 参数错误, ERR_DB 数据库错误
 */
int user_register(const char* username, const char* password);

/**
 * @brief 用户登录
 * @param username 用户名
 * @param password 密码
 * @return SUCCESS 成功, FAILURE 失败, ERR_PARAM 参数错误, ERR_DB 数据库错误
 */
int user_login(const char* username, const char* password);

/**
 * @brief 用户注销
 * @param username 用户名
 * @return SUCCESS 成功, FAILURE 失败, ERR_PARAM 参数错误, ERR_DB 数据库错误
 */
int user_logout(const char* username);
/** @} */

/**
 * @defgroup file_management 文件管理
 * @{
 */

/**
 * @brief 上传文件
 * @param filename 文件名
 * @param path 文件路径
 * @param hash 文件哈希值
 * @return SUCCESS 成功, FAILURE 失败, ERR_PARAM 参数错误, ERR_DB 数据库错误
 */
int file_upload(const char* filename, const char* path, const char* hash);

/**
 * @brief 下载文件
 * @param filename 文件名
 * @param path 文件路径
 * @return SUCCESS 成功, FAILURE 失败, ERR_PARAM 参数错误, ERR_DB 数据库错误
 */
int file_download(const char* filename, const char* path);

/**
 * @brief 删除文件
 * @param filename 文件名
 * @param path 文件路径
 * @return SUCCESS 成功, FAILURE 失败, ERR_PARAM 参数错误, ERR_DB 数据库错误
 */
int file_remove(const char* filename, const char* path);

/**
 * @brief 创建目录
 * @param dirname 目录名
 * @param path 目录路径
 * @param response  结果输出缓冲区
 * @param res_size  缓冲区大小
 * @return SUCCESS 成功, FAILURE 失败, ERR_PARAM 参数错误, ERR_DB 数据库错误
 */
int dir_mkdir(const char* dirname, const char* path, char *response, size_t res_size);

/**
 * @brief 删除目录
 * @param dirname 目录名
 * @param path 目录路径
 * @param response  结果输出缓冲区
 * @param res_size  缓冲区大小
 * @return SUCCESS 成功, FAILURE 失败, ERR_PARAM 参数错误, ERR_DB 数据库错误
 */
int dir_rmkdir(const char* dirname, const char* path, char *response, size_t res_size);

/**
 * @brief 列出目录内容
 * @param path 目录路径
 * @param response  结果输出缓冲区
 * @param res_size  缓冲区大小 
 * @return SUCCESS 成功, FAILURE 失败, ERR_PARAM 参数错误, ERR_DB 数据库错误
 */
int dir_ls(const char* path, char *response, size_t res_size);

/**
 * @brief 切换目录
 * @param path 目标路径
 * @param response  结果输出缓冲区
 * @param res_size  缓冲区大小
 * @return SUCCESS 成功, FAILURE 失败, ERR_PARAM 参数错误, ERR_DB 数据库错误
 */
int dir_cd(const char* path, char *response, size_t res_size);

/**
 * @brief 显示当前目录
 * @param response  结果输出缓冲区
 * @param res_size  缓冲区大小
 * @return SUCCESS 成功, FAILURE 失败, ERR_DB 数据库错误
 */
int dir_pwd(char *response, size_t res_size);
/** @} */

/**
 * @defgroup hash_calculation 哈希计算
 * @{
 */

/**
 * @brief 计算SHA1哈希值
 * @param data 数据
 * @param len 数据长度
 * @return 哈希值字符串（需释放内存）, NULL 失败
 */
char* compute_sha1(const char* data, size_t len);
/** @} */

/**
 * @defgroup database_query 数据库查询
 * @{
 */

/**
 * @brief 执行SQL查询
 * @param sql SQL语句
 * @param result 查询结果（需根据具体实现定义）
 * @return SUCCESS 成功, ERR_DB 失败
 */
int db_query(const char* sql, void* result);
/** @} */
/**
 * @defgroup error_handling 错误处理
 * @{
 */

/**
 * @brief 处理错误
 * @param message 错误消息
 */
void error_handling(const char* message);
/** @} */

/**
 * @defgroup logging 日志记录
 * @{
 */

/**
 * @brief 记录日志消息
 * @param message 消息
 */
void log_message(const char* message);

#endif // CLOUD_STORAGE_H


