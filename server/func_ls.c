#include "header.h"
/**
 * @brief 列出目录内容
 * @param path 目录路径
 * @param response  结果输出缓冲区
 * @param res_size  缓冲区大小 
 * @return SUCCESS 成功, FAILURE 失败, ERR_PARAM 参数错误, ERR_DB 数据库错误
 */
int dir_ls(MYSQL *mysql,const char* path, char *response, size_t res_size){
    if (path == NULL || response == NULL || res_size == 0) {
        snprintf(response,res_size,"%s\n","Parameter error");
        return ERR_PARAM;
    }
    // 拼接SQL查询语句
    char sql[256] = {0};
    int curr_parent_id = 0;
    snprintf(sql,sizeof(sql),"SELECT * FROM files WHERE parent_id  =  '%s'",curr_parent_id);
    char *result = NULL;
    int ret = db_query(mysql,sql,&result);
    if(ret != SUCCESS){
        snprintf(response,res_size,"%s\n","Database error");
        return ERR_DB;
    }
    // 解析查询结果
    printf("result:%s\n",result);
    // 释放结果集
    free(result);
    snprintf(response,res_size,"%s\n","List success");
    return SUCCESS;
}   