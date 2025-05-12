#include "header.h"
#include <openssl/sha.h>
/**
 * @brief 上传文件
 * @param filename 文件名
 * @param path 文件路径
 * @param hash 文件哈希值
 * @param sockfd 套接字
 * @param username 用户名
 * @return SUCCESS 成功, FAILURE 失败, ERR_PARAM 参数错误, ERR_DB 数据库错误
 */
int compute_sha1_from_memory(const char* data, size_t len, char *sha1_res);
/**
 * @brief 上传文件
 * @param mysql 数据库连接
 * @param username 用户名
 * @param filename 文件名
 * @param path 文件路径
 * @param file_content 文件内容
 * @param content_size 文件内容大小
 * @param response 结果输出缓冲区
 * @param res_size 缓冲区大小
 * @return 0 成功, -1 参数错误, FAILURE 失败, -3 数据库错误
 */





// 上传文件函数
int file_puts(MYSQL *mysql, const char *username, const char *filename,
              const char *path, const char *file_content, size_t content_size,
              char *response, size_t res_size) {
    printf("开始上传文件: %s\n", filename);
    char file_path[PATH_MAX] = {0};

    if (!filename || !path || !username || !file_content) {
        snprintf(response, res_size, "参数错误");
        return ERR_PARAM;
    }

    char hash[41] = {0};
    if (compute_sha1_from_memory(file_content, content_size, hash) != 0) {
        snprintf(response, res_size, "无法计算文件哈希值");
        return FAILURE;
    }

    printf("文件哈希值: %s\n", hash);

    // 检查是否已存在相同哈希文件
    char query[1024];
    snprintf(query, sizeof(query), "SELECT COUNT(*) FROM file_info WHERE hash='%s'", hash);
    MYSQL_RES *result;
    if (db_query(mysql, query, &result) != 0) {
        snprintf(response, res_size, "数据库查询失败");
        return ERR_DB;
    }

    MYSQL_ROW row = mysql_fetch_row(result);
    int count = atoi(row[0]);
    mysql_free_result(result);

    // 去除 path 尾部斜杠
    char normalized_path[PATH_MAX] = {0};
    strncpy(normalized_path, path, PATH_MAX - 1);
    size_t len = strlen(normalized_path);
    if (len > 0 && normalized_path[len - 1] == '/') {
        normalized_path[len - 1] = '\0';
    }

    // 构造文件路径
    snprintf(file_path, PATH_MAX, "%s/%s", normalized_path, hash);

    if (count > 0) {
        printf("文件已存在，秒传成功: %s\n", filename);
    } else {
        // 写入文件内容
        int fd = open(file_path, O_RDWR | O_CREAT | O_TRUNC, 0644);
        if (fd < 0) {
            snprintf(response, res_size, "创建文件失败: %s", strerror(errno));
            return FAILURE;
        }

        if (ftruncate(fd, content_size) != 0) {
            close(fd);
            return FAILURE;
        }

        void *mapped = mmap(NULL, content_size, PROT_WRITE, MAP_SHARED, fd, 0);
        if (mapped == MAP_FAILED) {
            close(fd);
            return FAILURE;
        }

        memcpy(mapped, file_content, content_size);
        munmap(mapped, content_size);
        close(fd);
    }

    // 插入数据库记录
    snprintf(query, sizeof(query),
             "INSERT INTO file_info (filename, username, hash, path) VALUES ('%s', '%s', '%s', '%s')",
             filename, username, hash, normalized_path);

    if (db_query(mysql, query, NULL) != 0) {
        snprintf(response, res_size, "数据库插入失败");
        if (count == 0) unlink(file_path); // 回滚
        return ERR_DB;
    }

    snprintf(response, res_size, "文件上传成功");
    return SUCCESS;
}

// 下载文件函数
int file_gets(MYSQL *mysql, const char *username, const char *filename,
              const char *path, char *response, size_t res_size,
              const char *userpath) {
    printf("开始下载文件: %s\n", filename);

    if (!filename || !path || !username || !userpath) {
        snprintf(response, res_size, "参数错误");
        return ERR_PARAM;
    }

    // 去除 path 尾部斜杠
    char temp_path[PATH_MAX] = {0};
    strncpy(temp_path, path, PATH_MAX - 1);
    size_t path_len = strlen(temp_path);
    if (path_len > 0 && temp_path[path_len - 1] == '/') {
        temp_path[path_len - 1] = '\0';
    }

    // 拼接用户文件夹路径
    char user_local_path[PATH_MAX] = {0};
    snprintf(user_local_path, PATH_MAX, "%s/%s", userpath, username);

    struct stat st;
    if (stat(user_local_path, &st) != 0) {
        if (mkdir(user_local_path, 0755) != 0) {
            snprintf(response, res_size, "创建用户文件夹失败: %s", strerror(errno));
            return FAILURE;
        }
    } else if (!S_ISDIR(st.st_mode)) {
        snprintf(response, res_size, "路径错误，不是目录");
        return ERR_PARAM;
    }

    // 构造文件路径
    char file_path[PATH_MAX] = {0};
    snprintf(file_path, PATH_MAX, "%s/%s", temp_path, filename);
    // 查询数据库确认文件记录
// 查询数据库确认文件记录并获取 hash
    char query[1024];
    snprintf(query, sizeof(query),
            "SELECT hash FROM file_info WHERE filename='%s' AND username='%s'",
            filename, username);
    MYSQL_RES *result;
    if (db_query(mysql, query, &result) != 0) {
        snprintf(response, res_size, "数据库查询失败");
        return ERR_DB;
    }

    MYSQL_ROW row = mysql_fetch_row(result);
    if (!row || !row[0]) {
        snprintf(response, res_size, "✅");
        mysql_free_result(result);
        return FAILURE;
    }
    char hash[41] = {0};
    strncpy(hash, row[0], sizeof(hash) - 1); // 拷贝 hash
    mysql_free_result(result);

    // 用 hash 构造真实文件路径
    memset(file_path, 0, sizeof(file_path));
    snprintf(file_path, PATH_MAX, "%s/%s", temp_path, hash);

    // 检查文件是否存在
    if (stat(file_path, &st) != 0) {
        snprintf(response, res_size, "文件不存在: %s", file_path);
        return FAILURE;
    }

    int fd = open(file_path, O_RDONLY);
    if (fd < 0) {
        snprintf(response, res_size, "无法打开文件: %s", strerror(errno));
        return FAILURE;
    }

    off_t file_size = st.st_size;
    void *mapped = mmap(NULL, file_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (mapped == MAP_FAILED) {
        close(fd);
        return FAILURE;
    }

    if (res_size > (size_t)file_size) {
        memcpy(response, mapped, file_size);
    } else {
        snprintf(response, res_size, "响应缓冲区不足");
        munmap(mapped, file_size);
        close(fd);
        return FAILURE;
    }

    munmap(mapped, file_size);
    close(fd);

    snprintf(response + file_size, res_size - file_size, "文件下载完成: %s", filename);
    return SUCCESS;
}
/*
 * @brief 删除文件
 * @param mysql 数据库连接
 * @param username 用户名
 * @param filename 文件名
 * @param path 文件路径
 * @param response 结果输出缓冲区
 * @param res_size 缓冲区大小
 * @return SUCCESS 成功, FAILURE 失败, ERR_PARAM 参数错误, ERR_DB 数据库错误
 */
int file_remove(MYSQL *mysql, const char *username, const char *filename, 
               const char *path, char *response, size_t res_size) {
    printf("开始删除文件（用户名: %s，文件名: %s）\n", username, filename);
    
    // 参数校验
    if (!username || !filename) {
        snprintf(response, res_size, "参数错误");
        return ERR_PARAM;
    }

    // 查询数据库：获取 hash 和 path
    char query[1024];
    snprintf(query, sizeof(query), 
             "SELECT hash, path FROM file_info WHERE filename='%s' AND username='%s'", 
             filename, username);

    MYSQL_RES *result;
    if (db_query(mysql, query, &result) != SUCCESS) {
        snprintf(response, res_size, "数据库查询失败");
        return ERR_DB;
    }

    MYSQL_ROW row = mysql_fetch_row(result);
    if (!row || !row[0] || !row[1]) {
        snprintf(response, res_size, "未找到对应的文件记录");
        mysql_free_result(result);
        return FAILURE;
    }

    const char *hash = row[0];
    const char *sql_path = row[1];
    mysql_free_result(result);

    // 构造文件路径：path + hash（物理文件名为 hash）
    char file_path[PATH_MAX] = {0};
    snprintf(file_path, PATH_MAX, "%s/%s", sql_path, hash);

    // 删除数据库记录
    char del_sql[1024];
    snprintf(del_sql, sizeof(del_sql), 
             "DELETE FROM file_info WHERE filename='%s' AND username='%s'", 
             filename, username);
    if (db_query(mysql, del_sql, NULL) != SUCCESS) {
        snprintf(response, res_size, "数据库删除失败");
        return ERR_DB;
    }

    // 删除物理文件（如果存在）
    struct stat st;
    if (stat(file_path, &st) == 0) {
        if (unlink(file_path) != 0) {
            snprintf(response, res_size, "删除文件失败: %s, 错误: %s", file_path, strerror(errno));
            return FAILURE;
        }
    } else {
        printf("文件在文件系统中不存在: %s\n", file_path);
    }

    printf("文件删除成功: %s\n", filename);
    snprintf(response, res_size, "文件删除成功: %s", filename);
    return SUCCESS;
}

/**
 * @brief 创建目录
 * @param dirname 目录名
 * @param path 目录路径
 * @param response  结果输出缓冲区
 * @param res_size  缓冲区大小
 * @return SUCCESS 成功, FAILURE 失败, ERR_PARAM 参数错误, ERR_DB 数据库错误
 */
/**
 * @brief 计算SHA1哈希值
 * @param data 数据
 * @param len 数据长度
 * @param sha1_res 结果输出
 * @return 0成功 ERR_DB 失败
 */
int compute_sha1_from_memory(const char* data, size_t len, char *sha1_res) {
    SHA_CTX ctx;
    if (SHA1_Init(&ctx) != 1) {
        return ERR_DB;
    }
    if (SHA1_Update(&ctx, data, len) != 1) {
        return ERR_DB;
    }
    unsigned char md[20];
    if (SHA1_Final(md, &ctx) != 1) {
        return ERR_DB;
    }
    for (int i = 0; i < 20; i++) {
        sprintf(sha1_res + i * 2, "%02x", md[i]);
    }
    printf("sha1加密结果: %s\n", sha1_res);
    return 0;
}

/**
 * @brief 断点续传下载文件（支持续传）
 * @param mysql 数据库连接
 * @param username 用户名
 * @param filename 文件名
 * @param path 服务器上的文件路径
 * @param local_path 本地保存文件的路径
 * @param response 结果输出缓冲区（用于返回文件内容）
 * @param res_size 缓冲区大小
 * @param userpath 用户文件夹基础路径
 * @return SUCCESS 成功, FAILURE 失败, ERR_PARAM 参数错误
 */
int file_gets_resume(MYSQL *mysql, const char *username, const char *filename, 
                    const char *path, const char *local_path, 
                    char *response, size_t res_size, const char *userpath) {
    printf("开始断点续传下载文件: %s\n", filename);
    
    // 检查参数
    if (!filename || !path || !username || !userpath || !local_path) {
        snprintf(response, res_size, "参数错误");
        return ERR_PARAM;
    }
    
    // 拼接服务器上的用户目录路径
    char user_local_path[PATH_MAX] = {0};
    snprintf(user_local_path, PATH_MAX, "%s/%s", userpath, username);
    
    // 检查服务器上的用户目录是否存在
    struct stat st;
    if (stat(user_local_path, &st) != 0) {
        // 目录不存在，尝试创建
        if (mkdir(user_local_path, 0755) != 0) {
            printf("无法创建用户文件夹: %s, 错误: %s\n", user_local_path, strerror(errno));
            snprintf(response, res_size, "无法创建用户文件夹: %s", user_local_path);
            return FAILURE;
        }
        printf("已创建用户文件夹: %s\n", user_local_path);
    } else if (!S_ISDIR(st.st_mode)) {
        // 路径存在但不是目录
        snprintf(response, res_size, "路径错误: %s 不是目录", user_local_path);
        return ERR_PARAM;
    }

    // 构建服务器上的文件完整路径
    char server_file_path[PATH_MAX] = {0};
    snprintf(server_file_path, PATH_MAX, "%s/%s", path, filename);

    // 检查服务器上的文件是否存在
    if (stat(server_file_path, &st) != 0) {
        printf("服务器上文件不存在: %s\n", server_file_path);
        snprintf(response, res_size, "文件不存在: %s", server_file_path);
        return FAILURE;
    }
    
    // 获取服务器上文件的总大小
    off_t server_file_size = st.st_size;
    
    // 构建本地文件的完整路径
    char local_file_path[PATH_MAX] = {0};
    snprintf(local_file_path, PATH_MAX, "%s/%s", local_path, filename);
    
    // 检查本地文件是否存在，获取已下载部分的大小
    off_t already_downloaded = 0;
    if (stat(local_file_path, &st) == 0) {
        // 本地文件存在，获取其大小作为断点续传的起点
        already_downloaded = st.st_size;
        printf("本地文件已存在，已下载大小: %ld 字节\n", (long)already_downloaded);
        
        // 检查是否已经下载完成
        if (already_downloaded >= server_file_size) {
            printf("文件已完全下载，无需继续\n");
            snprintf(response, res_size, "文件已完全下载: %s", filename);
            return SUCCESS;
        }
    }
    
    // 查询数据库中是否存在文件记录
    char query[1024];
    snprintf(query, sizeof(query), 
             "SELECT COUNT(*) FROM file_info WHERE filename='%s' AND path='%s' AND username='%s'", 
             filename, path, username);
    MYSQL_RES *result;
    if (db_query(mysql, query, &result) != SUCCESS) {
        snprintf(response, res_size, "数据库查询失败");
        return ERR_DB;
    }
    MYSQL_ROW row = mysql_fetch_row(result);
    int count = atoi(row[0]);
    mysql_free_result(result);

    // 如果数据库中没有记录，返回错误
    if (count == 0) {
        snprintf(response, res_size, "文件不存在于数据库: %s", filename);
        return FAILURE;
    }
    
    // 打开服务器上的文件
    int server_fd = open(server_file_path, O_RDONLY);
    if (server_fd < 0) {
        snprintf(response, res_size, "无法打开服务器文件: %s, 错误: %s", 
                server_file_path, strerror(errno));
        return FAILURE;
    }
    
    // 打开或创建本地文件，使用追加模式
    int local_fd = open(local_file_path, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (local_fd < 0) {
        snprintf(response, res_size, "无法打开本地文件: %s, 错误: %s", 
                local_file_path, strerror(errno));
        close(server_fd);
        return FAILURE;
    }
    
    // 从断点处开始读取文件
    if (lseek(server_fd, already_downloaded, SEEK_SET) != already_downloaded) {
        snprintf(response, res_size, "无法设置文件偏移量: %s", strerror(errno));
        close(server_fd);
        close(local_fd);
        return FAILURE;
    }
    
    // 计算需要传输的剩余数据大小
    off_t remaining_size = server_file_size - already_downloaded;
    printf("需要传输的剩余数据大小: %ld 字节\n", (long)remaining_size);
    
    // 使用缓冲区复制文件数据
    char buffer[8192]; // 8KB缓冲区
    ssize_t bytes_read, bytes_written;
    off_t total_transferred = 0;
    
    while (total_transferred < remaining_size) {
        // 读取数据
        bytes_read = read(server_fd, buffer, sizeof(buffer));
        if (bytes_read <= 0) {
            if (bytes_read == 0) {
                // 文件结束
                break;
            } else {
                // 读取错误
                snprintf(response, res_size, "读取文件错误: %s", strerror(errno));
                close(server_fd);
                close(local_fd);
                return FAILURE;
            }
        }
        
        // 写入数据到本地文件
        bytes_written = write(local_fd, buffer, bytes_read);
        if (bytes_written != bytes_read) {
            snprintf(response, res_size, "写入文件错误: %s", strerror(errno));
            close(server_fd);
            close(local_fd);
            return FAILURE;
        }
        
        total_transferred += bytes_written;
        
        // 可以在这里添加进度显示
        printf("\r传输进度: %.2f%%", 
               (float)(already_downloaded + total_transferred) * 100 / server_file_size);
        fflush(stdout);
    }
    
    printf("\n");
    
    // 关闭文件
    close(server_fd);
    close(local_fd);
    
    // 将传输完成的部分复制到响应缓冲区（如果需要）
    snprintf(response, res_size, "断点续传成功，总大小: %ld 字节，本次传输: %ld 字节", 
             (long)server_file_size, (long)total_transferred);
    
    printf("断点续传完成: %s，总大小: %ld，本次: %ld\n", 
           filename, (long)server_file_size, (long)total_transferred);
    
    return SUCCESS;
}

