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
    if(!mysql_real_connect(mysql,"localhost","root","12345","my_database",0,NULL,0)){
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
// 执行数据库查询，并根据 SQL 类型将结果写入 result 指向的位置。
// 参数：
//   MYSQL *mysql     —— MySQL 连接句柄
//   const char* sql  —— 要执行的 SQL 查询语句
//   void* result     —— 输出参数，select 时为 MYSQL_RES**，insert/update/delete 时为 my_ulonglong*
// 返回值：
//   成功返回 SUCCESS，失败返回 ERR_DB
int db_query(MYSQL *mysql, const char* sql, void* result) {
    // 调用 mysql_query 执行 SQL 语句，若失败则返回错误码
    if (mysql_query(mysql, sql) != 0) {
        printf("[错误] SQL 执行失败: %s\n", sql);
        return ERR_DB;
    }

    // 去除 SQL 语句前的空格，获取实际的 SQL 操作类型（如 SELECT、INSERT）
    const char* trimmed_sql = sql;
    while (isspace(*trimmed_sql)) trimmed_sql++;

    // 提取 SQL 语句的第一个单词（操作类型），最大长度限制为 15 个字符
    char first_word[16] = {0};
    sscanf(trimmed_sql, "%15s", first_word);

    // 将操作类型转为小写，便于统一比较
    for (int i = 0; first_word[i]; ++i) first_word[i] = tolower(first_word[i]);

    // 若是 SELECT 查询，尝试读取结果集
    if (strcmp(first_word, "select") == 0) {
        // 将结果强制转换为 MYSQL_RES 的二级指针
        MYSQL_RES **res_ptr = (MYSQL_RES **)result;
        // 从服务器获取完整结果集
        *res_ptr = mysql_store_result(mysql);
        // 若结果为空或出错，返回错误码
        if (*res_ptr == NULL) {
            printf("[错误] 查询结果为空或出错\n");
            return ERR_DB;
        }
        return SUCCESS;
    } 
    // 若是数据更新类操作（INSERT、UPDATE、DELETE）
    else if (strcmp(first_word, "insert") == 0 || strcmp(first_word, "update") == 0 || strcmp(first_word, "delete") == 0) {
        // 获取受影响的行数
        my_ulonglong affected_rows = mysql_affected_rows(mysql);
        // 若查询受影响行数失败，返回错误码
        if (affected_rows == (my_ulonglong)-1) {
            return ERR_DB;
        }
        // 若 result 非空，将受影响的行数写入该位置
        if (result != NULL) {
            *(my_ulonglong *)result = affected_rows;
        }
        printf("[调试] db_query result: '%s'\n", result);
        return SUCCESS;
    }

    // 若不是已知的 SQL 类型，统一返回错误
    return ERR_DB;
}
