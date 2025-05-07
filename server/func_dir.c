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

//mkdir-待完成
int dir_mkdir(MYSQL *mysql, int pwd,const char* dirname, char *response, size_t res_size){
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
    snprintf(sql,sizeof(sql),"select path from file_info where id=%d;",pwd);
    if(db_query(mysql,sql,temp)!=SUCCESS){
        snprintf(response,res_size,"查询失败！");
        return ERR_DB;
    }
    //解析路径
    char *line=strchr(temp,'\n');//跳过表头
    if(!line||!line[1]){
        snprintf(response,res_size,"找不到父目录！");
        return FAILURE;
    }
    line++;
    char parent_path[CHAR_MAX]={0};
    sscanf(line,"%s",parent_path);
    
    return SUCCESS;
}
