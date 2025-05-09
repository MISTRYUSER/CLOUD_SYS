#include "header.h"
/**
 * @brief 列出目录内容
 * @param path 目录路径
 * @param response  结果输出缓冲区
 * @param res_size  缓冲区大小 
 * @return SUCCESS 成功, FAILURE 失败, ERR_PARAM 参数错误, ERR_DB 数据库错误
 */
int dir_ls(MYSQL *mysql, char *response, size_t res_size, char *username) {
    // 参数检查
    if (response == NULL || res_size == 0) {
        snprintf(response, res_size, "%s\n", "Parameter error");
        return ERR_PARAM;
    }

    // 清空 response 缓冲区
    memset(response, 0, res_size);

    // 构造 SQL 查询
    char sql[256] = {0};
    snprintf(sql, sizeof(sql), "SELECT * FROM file_info WHERE id = %d AND (username = '%s' OR username = '0');", current_pwd_id, username);

    // 执行数据库查询
    MYSQL_RES *result = NULL;
    int ret = db_query(mysql, sql, &result);
    if (ret != SUCCESS || result == NULL) {
        snprintf(response, res_size, "%s\n", "Database error");
        if (result) mysql_free_result(result);
        return ERR_DB;
    }

    // 解析结果并写入 response
    MYSQL_ROW row;
    size_t offset = 0;
    while ((row = mysql_fetch_row(result)) && offset < res_size - 1) {
        size_t remaining = res_size - offset - 1; // 确保有剩余空间
        if (remaining > 0) {
            offset += snprintf(response + offset, remaining, "%s\n", row[1]); // 假设文件名在第 2 列
        }
    }
    mysql_free_result(result);

    // 处理空目录
    if (offset == 0) {
        snprintf(response, res_size, "目录为空\n");
    }
    return SUCCESS;
}