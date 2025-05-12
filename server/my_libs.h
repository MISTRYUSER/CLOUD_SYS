#ifndef MY_HEADER_H
#define MY_HEADER_H

#ifndef _GNU_SOURCE 
#define _GNU_SOURCE
#endif

/*======== 基础标准库（必须放在最前面） ========*/

#include <errno.h>      // errno 依赖此头文件
#include <stdio.h>      // 标准输入输出
#include <stdlib.h>     // exit() 依赖此头文件
#include <stdbool.h>    // 布尔类型支持
#include <stdint.h>     // 精确宽度整数类型
#include <string.h>     // 字符串操作
#include <ctype.h>      // 字符分类和转换
#include <time.h>       // 日期和时间函数
#include <limits.h>     // 数据类型限制常量
#include <locale.h>     // 本地化设置（多语言支持）

/*======== 其他头文件（顺序可调整） ========*/
#include <sys/stat.h>   // 文件状态信息
#include <fcntl.h>      // 文件控制选项
#include <unistd.h>     // POSIX系统调用
#include <dirent.h>     // 目录操作
#include <sys/mman.h>   // 内存映射文件
#include <sys/sendfile.h> // 零拷贝文件传输
#include <ftw.h>        // 文件树遍历（递归操作）
#include <pwd.h>        // 用户账户信息
#include <grp.h>        // 用户组信息
#include <shadow.h>     // 加密密码访问
#include <crypt.h>      // 密码加密函数
#include <sys/types.h>  // 基本系统数据类型
#include <sys/wait.h>   // 进程控制
#include <signal.h>     // 信号处理
#include <pthread.h>    // 线程操作
#include <sched.h>      // 线程调度控制
#include <sys/socket.h> // 套接字基础
#include <netinet/in.h> // IPV4/IPV6协议
#include <arpa/inet.h>  // 地址转换函数
#include <netdb.h>      // 主机名解析
#include <ifaddrs.h>    // 网络接口信息
#include <sys/select.h> // select多路复用
#include <sys/epoll.h>  // epoll多路复用(Linux)
#include <poll.h>       // poll机制
#include <syslog.h>     // 系统日志记录
#include <sys/time.h>   // 高精度时间获取
#include <sys/resource.h> // 资源限制控制
#include <sys/uio.h>    // 向量I/O操作
#include <zlib.h>       // gzip压缩支持
#include <mysql/mysql.h>
/*======== 宏定义（确保依赖的头文件已包含） ========*/
#define ARGS_CHECK(argc, expected) \
    do { \
        if ((argc) != (expected)) { \
            fprintf(stderr, "参数数量错误！需要%d个参数\n", (expected)); \
            exit(EXIT_FAILURE); \
        } \
    } while (0)

#define ERROR_CHECK(ret, error_flag, msg) \
    do { \
        if ((ret) == (error_flag)) { \
            perror(msg); \
            exit(EXIT_FAILURE); \
        } \
    } while (0)

#define THREAD_ERROR_CHECK(ret, msg) \
    do { \
        if ((ret) != 0) { \
            fprintf(stderr, "%s: %s\n", msg, strerror(ret)); \
        } \
    } while (0)

#define MALLOC_CHECK(ptr, msg) \
    do { \
        if ((ptr) == NULL) { \
            fprintf(stderr, "%s: 内存分配失败\n", msg); \
            exit(EXIT_FAILURE); \
        } \
    } while (0)

#define FILE_CHECK(fp, msg) \
    do { \
        if ((fp) == NULL) { \
            fprintf(stderr, "%s: %s\n", msg, strerror(errno)); \
            exit(EXIT_FAILURE); \
        } \
    } while (0)

#define NETWORK_CHECK(ret, msg) \
    do { \
        if ((ret) < 0) { \
            fprintf(stderr, "%s: %s\n", msg, strerror(errno)); \
            exit(EXIT_FAILURE); \
        } \
    } while (0)

#endif
