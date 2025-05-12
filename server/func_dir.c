#include "header.h"

//定义一个字段提取函数，每次select path后通过这个函数可以得到正确的path字符串
void extract_path(const char *src, char *dst, size_t dst_size) {
    printf("[extract_path] 原始字符串:\n%s\n", src);
    
    //跳过表头
    const char *line = strchr(src, '\n');
    if (!line || !line[1]) {
        printf("[extract_path] 无法找到有效数据行，返回空路径\n");
        dst[0] = '\0';
        return;
    }
    line++;
    
    //从line中读到第一个制表符或者换行符
    const char *end = strpbrk(line, "\t\n");
    size_t len = end ? (size_t)(end - line) : strlen(line);
    if (len >= dst_size) {
        len = dst_size - 1;
    }
    memcpy(dst, line, len);
    dst[len] = '\0';

    printf("[extract_path] 提取结果: %s\n", dst);
}

//pwd
int dir_pwd(MYSQL *mysql, char *path) {
    if (!mysql) return ERR_PARAM;

    char sql[CHAR_MAX] = {0};
    snprintf(sql, sizeof(sql), "select path from file_info where id = %d;", current_pwd_id);
    printf("[dir_pwd] 执行SQL: %s\n", sql);

    MYSQL_RES *res = NULL;
    if (db_query(mysql, sql, &res) != SUCCESS || res == NULL) {
        printf("[dir_pwd] 查询失败！\n");
        return ERR_DB;
    }

    MYSQL_ROW row = mysql_fetch_row(res);
    if (row && row[0]) {
        strncpy(path, row[0], PATH_MAX - 1);
        path[PATH_MAX - 1] = '\0';
    } else {
        printf("[dir_pwd] 无有效路径数据\n");
        path[0] = '\0';
    }
    mysql_free_result(res);

    printf("[dir_pwd] 当前路径: %s\n", path);
    return SUCCESS;
}
//mkdir
int dir_mkdir(MYSQL *mysql, const char *username, const char *dirname, char *response, size_t res_size) {
    if (!mysql || !dirname || strlen(dirname) == 0 || !response || res_size == 0) {
        snprintf(response, res_size, "参数错误!");
        return ERR_PARAM;
    }

    char sql[CHAR_MAX] = {0};
    snprintf(sql, sizeof(sql), "select path from file_info where id=%d;", current_pwd_id);
    printf("[dir_mkdir] 查询当前目录路径SQL: %s\n", sql);

    MYSQL_RES *res = NULL;
    if (db_query(mysql, sql, &res) != SUCCESS || res == NULL) {
        snprintf(response, res_size, "查询失败！");
        mysql_free_result(res);
        return ERR_DB;
    }

    char parent_path[CHAR_MAX] = {0};
    MYSQL_ROW row = mysql_fetch_row(res);
    if (row && row[0]) {
        strncpy(parent_path, row[0], sizeof(parent_path) - 1);
        parent_path[sizeof(parent_path) - 1] = '\0';
    } else {
        snprintf(response, res_size, "父路径无效！");
        mysql_free_result(res);
        return FAILURE;
    }
    mysql_free_result(res);

    char new_path[1024] = {0};
    if (strcmp(parent_path, "/") == 0) {
        snprintf(new_path, sizeof(new_path), "/%s", dirname);
    } else {
        // 检查parent_path是否以"/"结尾
        if (parent_path[strlen(parent_path) - 1] == '/') {
            snprintf(new_path, sizeof(new_path), "%s%s", parent_path, dirname);
        } else {
            snprintf(new_path, sizeof(new_path), "%s/%s", parent_path, dirname);
        }
    }
    printf("[dir_mkdir] 构造新路径: %s\n", new_path);

    // 检查目录是否已存在
    snprintf(sql, sizeof(sql),
             "select id from file_info where filename='%s' and parent_id=%d and username='%s' and type=2",
             dirname, current_pwd_id, username);
    printf("[dir_mkdir] 检查目录是否存在SQL: %s\n", sql);

    res = NULL;
    if (db_query(mysql, sql, &res) != SUCCESS || res == NULL) {
        snprintf(response, res_size, "数据库查询失败！");
        mysql_free_result(res);
        return ERR_DB;
    }
    if (mysql_num_rows(res) > 0) {
        snprintf(response, res_size, "目录已存在！");
        mysql_free_result(res);
        return FAILURE;
    }
    mysql_free_result(res);

    // 插入新目录
    snprintf(sql, sizeof(sql),
             "insert into file_info(filename,username,parent_id,path,type) values('%s','%s',%d,'%s',2)",
             dirname, username, current_pwd_id, new_path);
    printf("[dir_mkdir] 插入目录SQL: %s\n", sql);

    if (db_query(mysql, sql, NULL) != SUCCESS) {
        snprintf(response, res_size, "创建目录失败！");
        return ERR_DB;
    }

    snprintf(response, res_size, "目录 '%s' 创建成功！", new_path);
    printf("[dir_mkdir] %s\n", response);
    return SUCCESS;
}
//rmdir
int dir_rmdir(MYSQL *mysql, const char *username, const char *dirname, char *response, size_t res_size) {
    if (!mysql || !username || !dirname || !response || res_size == 0) {
        snprintf(response, res_size, "参数错误！");
        return ERR_PARAM;
    }

    char sql[1024] = {0};
    snprintf(sql, sizeof(sql),
             "select id from file_info where filename='%s' and parent_id=%d and type=2;",
             dirname, current_pwd_id);
    printf("[dir_rmdir] 查询目录ID SQL: %s\n", sql);

    MYSQL_RES *res = NULL;
    if (db_query(mysql, sql, &res) != SUCCESS || res == NULL) {
        snprintf(response, res_size, "数据库查询失败！");
        mysql_free_result(res);
        return ERR_DB;
    }

    MYSQL_ROW row = mysql_fetch_row(res);
    if (!row || !row[0]) {
        snprintf(response, res_size, "目录不存在！");
        mysql_free_result(res);
        return FAILURE;
    }
    int dir_id = atoi(row[0]);
    mysql_free_result(res);

    printf("[dir_rmdir] 目录ID: %d\n", dir_id);

    // 检查目录是否为空
    snprintf(sql, sizeof(sql),
             "select id from file_info where parent_id=%d;",
             dir_id);
    printf("[dir_rmdir] 检查目录是否为空SQL: %s\n", sql);

    res = NULL;
    if (db_query(mysql, sql, &res) != SUCCESS || res == NULL) {
        snprintf(response, res_size, "检查目录内容失败！");
        mysql_free_result(res);
        return ERR_DB;
    }
    if (mysql_num_rows(res) > 0) {
        snprintf(response, res_size, "目录不为空，无法删除！");
        mysql_free_result(res);
        return FAILURE;
    }
    mysql_free_result(res);

    // 删除目录
    snprintf(sql, sizeof(sql),
             "delete from file_info where id=%d;",
             dir_id);
    printf("[dir_rmdir] 删除目录SQL: %s\n", sql);

    if (db_query(mysql, sql, NULL) != SUCCESS) {
        snprintf(response, res_size, "删除目录失败！");
        return ERR_DB;
    }

    snprintf(response, res_size, "目录 '%s' 删除成功！", dirname);
    printf("[dir_rmdir] %s\n", response);
    return SUCCESS;
}