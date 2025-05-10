#include "header.h"
__thread int current_pwd_id = 0;//当前目录 id
int main()
{
    MYSQL *mysql = mysql_init(NULL);
    // 初始化数据库连接
    int n = db_init(mysql);
    if (n == 0)
    {
        printf("数据库连接成功！\n");
    }
    else
    {
        printf("数据库连接失败！\n");
        return -1;
    }
    char result[1024] = {0};
    char sql[1024] = {0};
    snprintf(sql, sizeof(sql), "select * from file_info");
    MYSQL_RES *query_result = NULL;
    int ret = db_query(mysql, sql, &query_result);
    if (ret == SUCCESS && query_result != NULL) {
        printf("查询结果:\n");
        MYSQL_ROW row;
        while ((row = mysql_fetch_row(query_result))) {
            for (int i = 0; i < mysql_num_fields(query_result); i++) {
                printf("%s\t", row[i] ? row[i] : "NULL");
            }
            printf("\n");
        }
        mysql_free_result(query_result);  // 释放结果集
    } else {
        printf("查询失败\n");
    }
    result[0] = '\0';
    dir_cd(mysql,"/test_public_dir",result,1024);
    printf("result:\n%s\n",result);
    
    dir_ls(mysql,result,1024,"alice");
    printf("当前路径有以下文件:\n%s\n",result);
    result[0] = '\0';
    char input[256];
    char response[1024];
    char path[1024];
    while (1) {
        printf("请输入命令(pwd/mkdir/rmdir/exit): ");
        scanf("%s", input);

        if (strcmp(input, "pwd") == 0) {
            if (dir_pwd(mysql, path) == SUCCESS) {
                printf("当前目录: %s\n", path);
            } else {
                printf("获取目录失败！\n");
            }
        } else if (strcmp(input, "mkdir") == 0) {
            char dirname[128];
            printf("请输入要创建的目录名: ");
            scanf("%s", dirname);
            if (dir_mkdir(mysql, "alice", dirname, response, sizeof(response)) == SUCCESS) {
                printf("成功: %s\n", response);
            } else {
                printf("失败: %s\n", response);
            }
        } else if (strcmp(input, "rmdir") == 0) {
            char dirname[128];
            printf("请输入要删除的目录名: ");
            scanf("%s", dirname);
            if (dir_rmdir(mysql, "alice", dirname, response, sizeof(response)) == SUCCESS) {
                printf("成功: %s\n", response);
            } else {
                printf("失败: %s\n", response);
            }
        } else if (strcmp(input, "exit") == 0) {
            break;
        } else {
            printf("未知命令。\n");
        }
    }
    int close = db_close(mysql); // 关闭数据库连接
    if(close == 0)
    {
        printf("数据库关闭成功！\n");
    }
    return 0;
}
