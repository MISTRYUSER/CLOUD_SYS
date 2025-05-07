#include "header.h"

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
int db_init(MYSQL *mysql){
    //初始化数据库连接
    if(!mysql)
    {
        printf("mysql_init failed\n");
        return ERR_DB;
    }
    if(!mysql_real_connect(mysql,"localhost","root","12345","my_database",3306,NULL,0)){
        printf("连接失败：%s\n",mysql_error(mysql));
        printf("mysql_real_connect failed\n");
        mysql_close(mysql);
        return ERR_DB;
    }
    printf("数据库连接成功！\n");
    return SUCCESS;
}

/**
 * @brief 关闭数据库连接
 * @return SUCCESS 成功, ERR_DB 失败
 */
int db_close(MYSQL *mysql){
    mysql_close(mysql);
    return SUCCESS;
}
/** @} */
/** @} */

/**
 * 通用数据库查询接口
 * @param mysql  MySQL 连接指针
 * @param sql    SQL 查询语句字符串
 * @param result 查询结果指针
 *               - 对于 SELECT：指向 MYSQL_RES* 的指针，调用方需释放结果集
 *               - 对于 INSERT/UPDATE/DELETE：指向 my_ulonglong*，返回受影响的行数
 *               - 若为 NULL，结果将自动释放或忽略
 * @return       成功返回 SUCCESS，失败返回 ERR_DB
 */
int db_query(MYSQL *mysql, const char* sql, void* result) {
    if (mysql_query(mysql, sql) != 0) {
        printf("[错误] SQL 执行失败: %s\n", sql);
        return ERR_DB;
    }

    const char* trimmed_sql = sql;
    while (isspace(*trimmed_sql)) trimmed_sql++;
    char first_word[16] = {0};
    sscanf(trimmed_sql, "%15s", first_word);
    for (int i = 0; first_word[i]; ++i) first_word[i] = tolower(first_word[i]);

    if (strcmp(first_word, "select") == 0) {
        MYSQL_RES *res = mysql_store_result(mysql);
        if (res == NULL) {
            printf("[错误] 查询结果为空或出错\n");
            return ERR_DB;
        }

        MYSQL_FIELD *fields = mysql_fetch_fields(res);
        int num_fields = mysql_num_fields(res);
        MYSQL_ROW row;

        char *output = (char *)result;
        size_t offset = 0;
        output[0] = '\0';

        for (int i = 0; i < num_fields; i++) {
            offset += snprintf(output + offset, 4096 - offset, "%s\t", fields[i].name);
        }
        offset += snprintf(output + offset, 4096 - offset, "\n");

        while ((row = mysql_fetch_row(res))) {
            for (int i = 0; i < num_fields; i++) {
                const char *val = row[i] ? row[i] : "NULL";
                offset += snprintf(output + offset, 4096 - offset, "%s\t", val);
            }
            offset += snprintf(output + offset, 4096 - offset, "\n");
        }

        mysql_free_result(res);
        return SUCCESS;
    }

    else if (strcmp(first_word, "insert") == 0 || strcmp(first_word, "update") == 0 || strcmp(first_word, "delete") == 0) {
        my_ulonglong affected_rows = mysql_affected_rows(mysql);
        if (affected_rows == (my_ulonglong)-1) {
            return ERR_DB;
        }

        if (result != NULL) {
            snprintf((char*)result, 128, "受影响行数：%llu\n", affected_rows);
        }

        return SUCCESS;
    }

    return ERR_DB;
}