#include "header.h"
/**
 * @brief 获取当前路径
 * @param mysql 数据库连接
 * @param path 结果输出路径
 * @param path_size 路径大小
 */
int get_current_path(MYSQL *mysql, char *path, size_t path_size) {
    if (!mysql || !path || path_size == 0) {
        return ERR_PARAM;
    }
    
    char sql[256] = {0};
    snprintf(sql, sizeof(sql), "SELECT path FROM file_info WHERE id = %d", current_pwd_id);
    
    MYSQL_RES *result = NULL;
    if (db_query(mysql, sql, &result) != SUCCESS || result == NULL) {
        if (result) mysql_free_result(result);
        return ERR_DB;
    }
    
    MYSQL_ROW row = mysql_fetch_row(result);
    if (row && row[0]) {
        strncpy(path, row[0], path_size - 1);
        path[path_size - 1] = '\0';
    } else {
        path[0] = '\0';
        mysql_free_result(result);
        return FAILURE;
    }
    
    mysql_free_result(result);
    return SUCCESS;
}