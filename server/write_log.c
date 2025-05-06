#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <stdlib.h>
#include <pthread.h>

// 日志级别定义
typedef enum {
    LOG_INFO,
    LOG_WARNING,
    LOG_ERROR
} LogLevel;

// 日志级别对应的字符串
const char* LogLevelStr[] = {
    "[INFO]",
    "[WARNING]",
    "[ERROR]"
};

// 线程安全的写日志函数
void write_log(LogLevel level, const char *fmt, ...) {
    // 打开日志文件
    FILE *fp = fopen("server.log", "a");
    if (!fp) {
        perror("Error opening log file");
        return; // 打开文件失败，直接返回
    }

    // 获取当前时间
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    char timebuf[32];
    strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", tm);

    // 获取可变参数
    va_list args;
    va_start(args, fmt);

    // 锁定文件以避免竞争条件（线程安全）
    static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_lock(&log_mutex);

    // 写入日志到文件
    fprintf(fp, "[%s] ", timebuf);
    fprintf(fp, "%s ", LogLevelStr[level]);  // 写入日志级别
    vfprintf(fp, fmt, args);
    fprintf(fp, "\n");

    // 解锁文件
    pthread_mutex_unlock(&log_mutex);

    // 结束参数处理
    va_end(args);

    // 关闭文件
    fclose(fp);
}

