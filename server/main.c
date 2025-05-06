#include "header.h"
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
    char sql[1024] = "INSERT INTO file_info (filename, username, parent_id, hash, path, type) VALUES ('doc.txt', 'alice', 0, 'abc123', '/docs/doc.txt', 1)";
    db_query(mysql,sql,result);
    printf("result:%s\n",result);
    int close = db_close(mysql); // 关闭数据库连接
    if(close == 0)
    {
        printf("数据库关闭成功！\n");
    }
    return 0;

}