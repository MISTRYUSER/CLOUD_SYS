#ifndef FILESERVER_H
#define FILESERVER_H

#include <mysql/mysql.h>
#include <sys/types.h>

// 常量定义
#define SUCCESS 0
#define FAILURE -1
#define ERR_PARAM -2
#define ERR_DB -3
#define MAX_FILE_SIZE (100 * 1024 * 1024)  // 100MB

#ifdef __cplusplus
extern "C" {
#endif

// ====================== 日志接口 ====================== //
typedef enum {
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR
} log_level_t;

void log_message(log_level_t level, const char* fmt, ...);

// ====================== 工具函数 ====================== //
// SHA1 计算
char* compute_sha1(const char* data, size_t len);

// 进度读取/保存
long read_upload_progress(const char* filename);
void save_upload_progress(const char* filename, long progress);
long read_download_progress(const char* filename);
void save_download_progress(const char* filename, long progress);

// ====================== 上传相关 ====================== //
int send_data_to_server(int sockfd, const void* data, size_t size);
int upload_chunk(int sockfd, void* chunk_data, size_t chunk_size);
int upload_file(int sockfd, int file_fd, const char* hash, long start_offset);
int file_upload(int sockfd, const char* filename, const char* path, const char* hash);

// ====================== 下载相关 ====================== //
int download_chunk_from_server(int sockfd, char* buffer, size_t buffer_size);
int download_file_from_server(int sockfd, int file_fd);
int file_download(int sockfd, const char* filename, const char* path);

#ifdef __cplusplus
}
#endif

#endif // FILESERVER_H
