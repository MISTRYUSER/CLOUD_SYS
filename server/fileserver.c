#include "fileserver.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <arpa/inet.h>
#include <time.h>

// ====================== 日志实现 ====================== //

static const char* log_level_str[] = { "INFO", "WARN", "ERROR" };

void log_message(log_level_t level, const char* fmt, ...) {
    FILE* log_fp = fopen("fileserver.log", "a");
    if (!log_fp) return;
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    char time_buf[32];
    strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", tm_info);

    fprintf(log_fp, "[%s] [%s] ", time_buf, log_level_str[level]);
    va_list args;
    va_start(args, fmt);
    vfprintf(log_fp, fmt, args);
    va_end(args);
    fprintf(log_fp, "\n");
    fclose(log_fp);
}

// ====================== 工具函数实现 ====================== //

long read_upload_progress(const char* filename) {
    char progress_file[256];
    snprintf(progress_file, sizeof(progress_file), "%s.progress", filename);
    FILE* file = fopen(progress_file, "r");
    if (file == NULL) {
        log_message(LOG_INFO, "No upload progress file for %s. Start from 0.", filename);
        return 0;
    }
    long progress = 0;
    fscanf(file, "%ld", &progress);
    fclose(file);
    log_message(LOG_INFO, "Read upload progress for %s: %ld", filename, progress);
    return progress;
}

void save_upload_progress(const char* filename, long progress) {
    char progress_file[256];
    snprintf(progress_file, sizeof(progress_file), "%s.progress", filename);
    FILE* file = fopen(progress_file, "w");
    if (file != NULL) {
        fprintf(file, "%ld", progress);
        fclose(file);
        log_message(LOG_INFO, "Saved upload progress for %s: %ld", filename, progress);
    }
}

long read_download_progress(const char* filename) {
    char progress_file[256];
    snprintf(progress_file, sizeof(progress_file), "%s.progress", filename);
    FILE* file = fopen(progress_file, "r");
    if (file == NULL) {
        log_message(LOG_INFO, "No download progress file for %s. Start from 0.", filename);
        return 0;
    }
    long progress = 0;
    fscanf(file, "%ld", &progress);
    fclose(file);
    log_message(LOG_INFO, "Read download progress for %s: %ld", filename, progress);
    return progress;
}

void save_download_progress(const char* filename, long progress) {
    char progress_file[256];
    snprintf(progress_file, sizeof(progress_file), "%s.progress", filename);
    FILE* file = fopen(progress_file, "w");
    if (file != NULL) {
        fprintf(file, "%ld", progress);
        fclose(file);
        log_message(LOG_INFO, "Saved download progress for %s: %ld", filename, progress);
    }
}

// ====================== 上传相关实现 ====================== //

int send_data_to_server(int sockfd, const void* data, size_t size) {
    ssize_t bytes_sent = send(sockfd, data, size, 0);
    if (bytes_sent == -1) {
        perror("Send failed");
        log_message(LOG_ERROR, "Send to server failed: %s", strerror(errno));
        return FAILURE;
    }
    log_message(LOG_INFO, "Sent %zd bytes to server", bytes_sent);
    return SUCCESS;
}

int upload_chunk(int sockfd, void* chunk_data, size_t chunk_size) {
    int ret = send_data_to_server(sockfd, chunk_data, chunk_size);
    if (ret == SUCCESS)
        log_message(LOG_INFO, "Chunk of size %zu uploaded.", chunk_size);
    else
        log_message(LOG_ERROR, "Chunk upload failed.");
    return ret;
}

int upload_file(int sockfd, int file_fd, const char* hash, long start_offset) {
    if (lseek(file_fd, start_offset, SEEK_SET) == -1) {
        perror("Seek to offset failed");
        log_message(LOG_ERROR, "Seek to offset %ld failed: %s", start_offset, strerror(errno));
        return FAILURE;
    }
    ssize_t bytes_sent = send(sockfd, hash, strlen(hash), 0);
    if (bytes_sent == -1) {
        perror("Send hash failed");
        log_message(LOG_ERROR, "Send hash failed: %s", strerror(errno));
        return FAILURE;
    }

    char buffer[1024];
    ssize_t bytes_read;
    while ((bytes_read = read(file_fd, buffer, sizeof(buffer))) > 0) {
        if (send_data_to_server(sockfd, buffer, bytes_read) != SUCCESS) {
            log_message(LOG_ERROR, "Failed to send file data.");
            return FAILURE;
        }
    }
    if (bytes_read == -1) {
        perror("Read file failed");
        log_message(LOG_ERROR, "Read file failed: %s", strerror(errno));
        return FAILURE;
    }
    log_message(LOG_INFO, "File uploaded successfully.");
    return SUCCESS;
}

int file_upload(MYSQL *mysql, const char* filename, const char* path, const char* hash) {
    MYSQL* conn = mysql_init(NULL);
    if (conn == NULL) {
        log_message(LOG_ERROR, "MySQL init failed.");
        return ERR_DB;
    }
    if (mysql_real_connect(conn, "localhost", "root", "362362", "my_database", 0, NULL, 0) == NULL) {
        log_message(LOG_ERROR, "MySQL connect failed: %s", mysql_error(conn));
        mysql_close(conn);
        return ERR_DB;
    }

    char query[512];
    snprintf(query, sizeof(query), "SELECT id FROM file_info WHERE hash='%s'", hash);
    if (mysql_query(conn, query)) {
        log_message(LOG_ERROR, "MySQL query failed: %s", mysql_error(conn));
        mysql_close(conn);
        return ERR_DB;
    }
    MYSQL_RES* res = mysql_store_result(conn);
    if (res == NULL) {
        log_message(LOG_ERROR, "MySQL store_result failed: %s", mysql_error(conn));
        mysql_close(conn);
        return ERR_DB;
    }
    if (mysql_num_rows(res) > 0) {
        snprintf(query, sizeof(query),
                 "INSERT INTO file_info (filename, username, parent_id, hash, path, type) "
                 "VALUES ('%s', 'user_name', 0, '%s', '%s', 1)",
                 filename, hash, path);
        if (mysql_query(conn, query)) {
            log_message(LOG_ERROR, "MySQL insert (秒传) failed: %s", mysql_error(conn));
            mysql_free_result(res);
            mysql_close(conn);
            return ERR_DB;
        }
        log_message(LOG_INFO, "秒传成功: %s", filename);
        mysql_free_result(res);
        mysql_close(conn);
        return SUCCESS;
    }

    int file_fd = open(path, O_RDONLY);
    if (file_fd == -1) {
        log_message(LOG_ERROR, "Open file for upload failed: %s", strerror(errno));
        mysql_free_result(res);
        mysql_close(conn);
        return ERR_PARAM;
    }
    struct stat file_stat;
    fstat(file_fd, &file_stat);
    off_t file_size = file_stat.st_size;
    long uploaded = read_upload_progress(filename);

    if (uploaded < file_size) {
        if (send(sockfd, filename, strlen(filename), 0) == -1) {
            perror("Send request failed");
            log_message(LOG_ERROR, "Send upload request failed: %s", strerror(errno));
            close(file_fd);
            mysql_free_result(res);
            mysql_close(conn);
            return FAILURE;
        }

        if (file_size > MAX_FILE_SIZE) {
            void* file_map = mmap(NULL, file_size, PROT_READ, MAP_PRIVATE, file_fd, 0);
            if (file_map == MAP_FAILED) {
                log_message(LOG_ERROR, "mmap failed: %s", strerror(errno));
                close(file_fd);
                mysql_free_result(res);
                mysql_close(conn);
                return ERR_PARAM;
            }
            int chunk_size = 1024 * 1024;
            int chunk_count = (file_size + chunk_size - 1) / chunk_size;
            for (int i = 0; i < chunk_count; i++) {
                off_t offset = i * chunk_size;
                size_t chunk_size_to_send = (file_size - offset < chunk_size) ? file_size - offset : chunk_size;
                void* chunk_data = (char*)file_map + offset;
                if (upload_chunk(sockfd, chunk_data, chunk_size_to_send) != SUCCESS) {
                    log_message(LOG_ERROR, "Failed to upload chunk %d/%d", i+1, chunk_count);
                    munmap(file_map, file_size);
                    close(file_fd);
                    mysql_free_result(res);
                    mysql_close(conn);
                    return FAILURE;
                }
                uploaded += chunk_size_to_send;
                save_upload_progress(filename, uploaded);
            }
            munmap(file_map, file_size);
        } else {
            if (upload_file(sockfd, file_fd, hash, uploaded) != SUCCESS) {
                log_message(LOG_ERROR, "Small file upload failed.");
                close(file_fd);
                mysql_free_result(res);
                mysql_close(conn);
                return FAILURE;
            }
        }
    }

    snprintf(query, sizeof(query),
             "INSERT INTO file_info (filename, username, parent_id, hash, path, type) "
             "VALUES ('%s', 'user_name', 0, '%s', '%s', 1)",
             filename, hash, path);

    if (mysql_query(conn, query)) {
        log_message(LOG_ERROR, "MySQL insert (normal upload) failed: %s", mysql_error(conn));
        close(file_fd);
        mysql_free_result(res);
        mysql_close(conn);
        return ERR_DB;
    }
    log_message(LOG_INFO, "Upload finished and DB record inserted: %s", filename);

    close(file_fd);
    mysql_free_result(res);
    mysql_close(conn);
    return SUCCESS;
}

// ====================== 下载相关实现 ====================== //

int download_chunk_from_server(int sockfd, char* buffer, size_t buffer_size) {
    ssize_t bytes_received = recv(sockfd, buffer, buffer_size, 0);
    if (bytes_received == -1) {
        perror("Receive chunk failed");
        log_message(LOG_ERROR, "Receive chunk failed: %s", strerror(errno));
        return FAILURE;
    }
    log_message(LOG_INFO, "Received chunk of size %zd bytes", bytes_received);
    return bytes_received;
}

int download_file_from_server(int sockfd, int file_fd) {
    char buffer[1024];
    ssize_t bytes_received;
    while ((bytes_received = recv(sockfd, buffer, sizeof(buffer), 0)) > 0) {
        ssize_t bytes_written = write(file_fd, buffer, bytes_received);
        if (bytes_written != bytes_received) {
            perror("Write to file failed");
            log_message(LOG_ERROR, "Write to file failed: %s", strerror(errno));
            return FAILURE;
        }
    }
    if (bytes_received == -1) {
        perror("Receive failed");
        log_message(LOG_ERROR, "Receive failed: %s", strerror(errno));
        return FAILURE;
    }
    log_message(LOG_INFO, "File download finished.");
    return SUCCESS;
}

int file_download(int sockfd, const char* filename, const char* path) {
    char* hash = compute_sha1(filename, strlen(filename));
    if (hash == NULL) {
        log_message(LOG_ERROR, "SHA1 calculation failed for %s", filename);
        return ERR_PARAM;
    }
    MYSQL* conn = mysql_init(NULL);
    if (conn == NULL) {
        log_message(LOG_ERROR, "MySQL init failed.");
        return ERR_DB;
    }
    if (mysql_real_connect(conn, "localhost", "root", "362362", "my_database", 0, NULL, 0) == NULL) {
        log_message(LOG_ERROR, "MySQL connect failed: %s", mysql_error(conn));
        mysql_close(conn);
        return ERR_DB;
    }
    char query[512];
    snprintf(query, sizeof(query), "SELECT path FROM file_info WHERE hash='%s'", hash);
    if (mysql_query(conn, query)) {
        log_message(LOG_ERROR, "MySQL query failed: %s", mysql_error(conn));
        mysql_close(conn);
        return ERR_DB;
    }
    MYSQL_RES* res = mysql_store_result(conn);
    if (res == NULL) {
        log_message(LOG_ERROR, "MySQL store_result failed: %s", mysql_error(conn));
        mysql_close(conn);
        return ERR_DB;
    }
    if (mysql_num_rows(res) == 0) {
        log_message(LOG_WARN, "File not found in DB for download: %s", filename);
        mysql_free_result(res);
        mysql_close(conn);
        return FAILURE;
    }
    MYSQL_ROW row = mysql_fetch_row(res);
    const char* file_path = row[0];
    long downloaded = read_download_progress(filename);
    int file_fd = open(path, O_WRONLY | O_CREAT | O_APPEND, 0666);
    if (file_fd == -1) {
        log_message(LOG_ERROR, "Open file for download failed: %s", strerror(errno));
        mysql_free_result(res);
        mysql_close(conn);
        return FAILURE;
    }
    struct stat file_stat;
    fstat(file_fd, &file_stat);
    off_t file_size = file_stat.st_size;

    if (send(sockfd, filename, strlen(filename), 0) == -1) {
        perror("Send request failed");
        log_message(LOG_ERROR, "Send download request failed: %s", strerror(errno));
        close(file_fd);
        mysql_free_result(res);
        mysql_close(conn);
        return FAILURE;
    }
    if (send(sockfd, &downloaded, sizeof(downloaded), 0) == -1) {
        perror("Send download progress failed");
        log_message(LOG_ERROR, "Send download progress failed: %s", strerror(errno));
        close(file_fd);
        mysql_free_result(res);
        mysql_close(conn);
        return FAILURE;
    }
    if (file_size > MAX_FILE_SIZE) {
        size_t chunk_size = 1024 * 1024;
        char buffer[chunk_size];
        ssize_t bytes_received;
        while ((bytes_received = download_chunk_from_server(sockfd, buffer, chunk_size)) > 0) {
            ssize_t bytes_written = write(file_fd, buffer, bytes_received);
            if (bytes_written != bytes_received) {
                log_message(LOG_ERROR, "Write to file failed during chunked download.");
                close(file_fd);
                mysql_free_result(res);
                mysql_close(conn);
                return FAILURE;
            }
            downloaded += bytes_received;
            save_download_progress(filename, downloaded);
        }
        if (bytes_received == -1) {
            log_message(LOG_ERROR, "Download chunk from server failed.");
            close(file_fd);
            mysql_free_result(res);
            mysql_close(conn);
            return FAILURE;
        }
    } else {
        if (download_file_from_server(sockfd, file_fd) != SUCCESS) {
            log_message(LOG_ERROR, "Small file download failed.");
            close(file_fd);
            mysql_free_result(res);
            mysql_close(conn);
            return FAILURE;
        }
    }
    close(file_fd);
    mysql_free_result(res);
    mysql_close(conn);
    log_message(LOG_INFO, "Download completed for file %s", filename);
    return SUCCESS;
}
