#ifndef MY_HEADER_H
#define MY_HEADER_H

#ifndef _GNU_SOURCE 
#define _GNU_SOURCE
#endif


/************************
 * 网盘项目头文件列表
 ​************************/

/*======== 基础标准库 ========*/
#include <stdio.h>      // 标准输入输出
#include <stdlib.h>     // 内存管理、系统命令
#include <stdbool.h>    // 布尔类型支持
#include <stdint.h>     // 精确宽度整数类型
#include <string.h>     // 字符串操作
#include <ctype.h>      // 字符分类和转换
#include <time.h>       // 日期和时间函数
#include <errno.h>      // 错误号定义
#include <limits.h>     // 数据类型限制常量
#include <locale.h>     // 本地化设置（多语言支持）

/*======== 文件系统操作 ========*/
#include <sys/stat.h>   // 文件状态信息
#include <fcntl.h>      // 文件控制选项
#include <unistd.h>     // POSIX系统调用
#include <dirent.h>     // 目录操作
#include <sys/mman.h>   // 内存映射文件
#include <sys/sendfile.h> // 零拷贝文件传输
#include <ftw.h>        // 文件树遍历（递归操作）

/*======== 用户权限管理 ========*/
#include <pwd.h>        // 用户账户信息
#include <grp.h>        // 用户组信息
#include <shadow.h>     // 加密密码访问
#include <crypt.h>      // 密码加密函数

/*======== 进程/线程控制 ========*/
#include <sys/types.h>  // 基本系统数据类型
#include <sys/wait.h>   // 进程控制
#include <signal.h>     // 信号处理
#include <pthread.h>    // 线程操作
#include <sched.h>      // 线程调度控制

/*======== 网络通信 ========*/
#include <sys/socket.h> // 套接字基础
#include <netinet/in.h> // IPV4/IPV6协议
#include <arpa/inet.h>  // 地址转换函数
#include <netdb.h>      // 主机名解析
#include <ifaddrs.h>    // 网络接口信息

/*======== 高级I/O复用 ========*/
#include <sys/select.h> // select多路复用
#include <sys/epoll.h>  // epoll多路复用(Linux)
#include <poll.h>       // poll机制

/*======== 数据存储 ========*/
#include <sqlite3.h>    // 嵌入式数据库（比MySQL轻量）
#include <db.h>         // Berkeley DB（可选）

/*======== 安全加密 ========*/
#include <openssl/ssl.h>    // SSL/TLS支持
#include <openssl/err.h>    // OpenSSL错误处理
#include <openssl/sha.h>    // SHA哈希算法

/*======== 系统服务 ========*/
#include <syslog.h>     // 系统日志记录
#include <sys/time.h>   // 高精度时间获取
#include <sys/resource.h> // 资源限制控制
#include <sys/uio.h>    // 向量I/O操作

/*======== 数据压缩 ========*/
#include <zlib.h>       // gzip压缩支持

/*======== 配置解析 ========*/
#include <json-c/json.h> // JSON配置解析

// 参数数量检查（用于检查命令行参数）
#define ARGS_CHECK(argc, expected) \
    do { \
        if ((argc) != (expected)) { \
            fprintf(stderr, "参数数量错误！需要%d个参数\n", (expected)); \
            exit(-1); \
        } \
    } while (0)

// 通用错误检查（用于系统调用返回值检查）
#define ERROR_CHECK(ret, error_flag, msg) \
    do { \
        if ((ret) == (error_flag)) { \
            perror(msg); \  // 自动附带errno对应的错误描述
            exit(-1); \
        } \
    } while (0)

// 线程错误检查（专用于pthread函数返回值检查）
#define THREAD_ERROR_CHECK(ret, msg) \
    do { \
        if ((ret) != 0) { \
            fprintf(stderr, "%s: %s\n", msg, strerror(ret)); \  // 使用线程专用错误码
        } \
    } while (0)

// 内存分配检查（用于malloc/calloc等）
#define MALLOC_CHECK(ptr, msg) \
    do { \
        if ((ptr) == NULL) { \
            fprintf(stderr, "%s: 内存分配失败\n", msg); \
            exit(-1); \
        } \
    } while (0)

// 文件操作检查（用于fopen等文件操作）
#define FILE_CHECK(fp, msg) \
    do { \
        if ((fp) == NULL) { \
            fprintf(stderr, "%s: %s\n", msg, strerror(errno)); \
            exit(-1); \
        } \
    } while (0)

// 网络操作检查（用于socket/connect等网络操作）
#define NETWORK_CHECK(ret, msg) \
    do { \
        if ((ret) < 0) { \
            fprintf(stderr, "%s: %s\n", msg, strerror(errno)); \
            exit(-1); \
        } \
    } while (0)

#endif