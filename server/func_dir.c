#include "header.h"
//定义一个字段提取函数，每次select path后通过这个函数可以得到正确的path字符串
void extract_path(const char *src,char *dst,size_t dst_size){
    //跳过表头
    const char *line=strchr(src,'\n');
    if(!line||!line[1]){
        dst[0]='\0';
        return;
    }
    line++;
    //从line中读到第一个制表符或者换行符
    const char *end=strpbrk(line,"\t\n");
    size_t len=end?(size_t)(end-line):strlen(line);
    if(len>=dst_size){
        len=dst_size -1;
    }
    memcpy(dst,line,len);
    dst[len]='\0';
}
//pwd
int dir_pwd(MYSQL *mysql, char *path){
    //客户端连接时，pwd为当前客户端的/home文件夹id
    //整数pwd，代表当前所在文件夹的文件id，用户连接初始为0，每次cd后更改pwd数值
    //参数检验
    if(!mysql) return ERR_PARAM;
    char sql[CHAR_MAX]={0};
    char temp[CHAR_MAX]={0};
    //查询path
    snprintf(sql,sizeof(sql),"select path from file_info where id =%d;",current_pwd_id);
    if(db_query(mysql,sql,temp)!=SUCCESS){
        //查询失败
        printf("查询失败！\n");
        return ERR_DB;
    }
    //把裁切完成的path传出
    extract_path(temp,path,sizeof(path));

    return SUCCESS;
}

//mkdir
int dir_mkdir(MYSQL *mysql,const char *username, const char* dirname, char *response, size_t res_size){
    //创建文件夹，先检查是否合法，如果合法就在文件元信息插入一条
    //先检查dirname是否合法
    if(!mysql || !dirname || strlen(dirname) == 0 || !response || res_size == 0){
        snprintf(response,res_size,"参数错误!");
        return ERR_PARAM;
    }
    //获取真实目录
    char sql[CHAR_MAX]={0};
    char temp[CHAR_MAX]={0};
    //获取当前目录路径
    snprintf(sql,sizeof(sql),"select path from file_info where id=%d;",current_pwd_id);
    if(db_query(mysql,sql,temp)!=SUCCESS){
        snprintf(response,res_size,"查询失败！");
        return ERR_DB;
    }
    //解析路径
    char parent_path[CHAR_MAX]={0};
    extract_path(temp,parent_path,sizeof(parent_path));
    if(strlen(parent_path)==0){
        snprintf(response,res_size,"父路径无效！");
        return FAILURE;
    }
    //构造新路径
    char new_path[1024]={0};
    if(strcmp(parent_path,"/")==0){
        snprintf(new_path,sizeof(new_path),"/%s",dirname);
    }
    else{
        snprintf(new_path,sizeof(new_path),"%s/%s",parent_path,dirname);
    }
    //检查是否已经存在相同目录
    snprintf(sql,sizeof(sql),"select id from file_info where filename ='%s'and parent_id=%d and username='%s' and type=2",dirname,current_pwd_id,username);
    memset(temp,0,sizeof(temp));
    if(db_query(mysql,sql,temp)!=SUCCESS){
        snprintf(response,res_size,"数据库查询失败！");
        return ERR_DB;
    }
    if(strchr(temp,'\n')&&strchr(temp,'\n')[1]){
        snprintf(response,res_size,"目录已存在！");
        return FAILURE;
    }
    //插入新目录
    snprintf(sql,sizeof(sql),
             "insert into file_info(filename,username,parent_id,path,type) values('%s','%s',%d,'%s',2)",
             dirname,username,current_pwd_id,new_path);
    if(db_query(mysql,sql,NULL)!=SUCCESS){
        snprintf(response, res_size, "创建目录失败！");
        return ERR_DB;
    }
    snprintf(response, res_size, "目录 '%s' 创建成功！", new_path);
    return SUCCESS;
}
//rmdir
int dir_rmdir(MYSQL *mysql,const char *username, const char* dirname, char *response, size_t res_size){
    //参数检验
    if (!mysql || !username || !dirname || !response || res_size == 0) {
        snprintf(response, res_size, "参数错误！");
        return ERR_PARAM;
    }
    char sql[1024] = {0}, temp[4096] = {0};
    //获取要删除的目录id
    snprintf(sql, sizeof(sql),
             "select id from file_info where filename='%s' and parent_id=%d and username='%s' and type=2;",
             dirname, current_pwd_id, username);
    if (db_query(mysql, sql, temp) != SUCCESS) {
        snprintf(response, res_size, "数据库查询失败！");
        return ERR_DB;
    }
    char dir_id_str[32] = {0};
    extract_path(temp, dir_id_str, sizeof(dir_id_str));
    if(strlen(dir_id_str)==0){
        snprintf(response, res_size, "目录不存在！");
        return FAILURE;
    }
    //确认存在，转str为int
    int dir_id=atoi(dir_id_str);
    //检查目录是否为空
    snprintf(sql, sizeof(sql),
             "select id from file_info where parent_id = %d and username = '%s';",
             dir_id, username);
    memset(temp,0,sizeof(temp));
    if (db_query(mysql, sql, temp) != SUCCESS) {
        snprintf(response, res_size, "检查目录内容失败！");
        return ERR_DB;
    }
    if (strchr(temp, '\n') && strchr(temp, '\n')[1]) {
        snprintf(response, res_size, "目录不为空，无法删除！");
        return FAILURE;
    }
    //删除目录
    snprintf(sql,sizeof(sql),
             "delete from file_info where id=%d and username ='%s';",
             dir_id,username);
    if (db_query(mysql, sql, NULL) != SUCCESS) {
        snprintf(response, res_size, "删除目录失败！");
        return ERR_DB;
    }
    snprintf(response, res_size, "目录 '%s' 删除成功！", dirname);

    return SUCCESS;
}
