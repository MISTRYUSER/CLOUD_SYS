#include "header.h"

// 线程局部变量，用于目录栈、当前路径和当前目录 ID
// 使用 _Thread_local 确保每个线程独立拥有这些变量，
// 提高多线程并发访问时的线程安全性
_Thread_local PathStack thread_path_stack = {{0}, 0}; // 目录数组，栈顶索引
_Thread_local char cur_v_path[PATH_MAX] = "/";       // 当前虚拟路径字符串
_Thread_local int current_dir_id = 0;                // 当前目录在数据库中的 ID（0 通常为根目录）

/**
 * @brief 初始化目录栈结构，用于管理目录层级
 * @return 0 表示成功，-1 表示失败（当前未定义具体失败场景）
 * @note 将栈顶指针设置为 -1 表示栈为空，虚拟路径初始化为根目录 "/"
 * @warning 使用 _Thread_local 变量 thread_path_stack 和 cur_v_path 确保线程安全
 */
int stack_init() {
    thread_path_stack.top = -1; // 设置栈顶指针为 -1，表示空栈
    current_dir_id = 0;         // 初始化当前目录 ID 为根目录（0）
    strcpy(cur_v_path, "/");    // 初始化虚拟路径为 "/"
    return 0;                   // 返回成功（无错误检查）
}

/**
 * @brief 将目录名称压入栈中，表示进入子目录
 * @param dir 要压入栈的目录名（以 null 结尾的字符串）
 * @return 0 表示成功，-1 表示失败（栈满、参数无效或内存分配失败）
 * @note 动态分配内存存储目录名，更新虚拟路径 cur_v_path，格式为 "/dir1/dir2"
 * @warning 使用 _Thread_local 变量确保线程安全
 */
int stack_push(const char *dir) {
    if (!dir || dir[0] == '\0') return -1; // 检查目录名是否有效且非空
    if (thread_path_stack.top >= MAX_STACK_SIZE - 1) return -1; // 检查栈是否已满

    // 分配内存并复制目录名
    char *dir_copy = strdup(dir);
    if (!dir_copy) return -1; // 检查内存分配是否失败

    thread_path_stack.top++; // 栈顶指针加一
    thread_path_stack.dir[thread_path_stack.top] = dir_copy;

    // 重建虚拟路径
    char temp[PATH_MAX];
    if (thread_path_stack.top == 0) {
        snprintf(temp, PATH_MAX, "/%s", dir); // 第一个元素，路径为 "/dir"
    } else {
        snprintf(temp, PATH_MAX, "%s/%s", cur_v_path, dir); // 追加到当前路径
    }
    strncpy(cur_v_path, temp, PATH_MAX);
    cur_v_path[PATH_MAX - 1] = '\0'; // 确保路径以 null 结尾

    return 0; // 返回成功
}

/**
 * @brief 弹出目录栈中的一个目录名，表示返回上一级目录
 * @return 0 表示成功，-1 表示失败（栈为空）
 * @note 释放弹出目录的内存，更新虚拟路径；栈为空时路径恢复为 "/"
 * @warning 使用 _Thread_local 变量确保线程安全
 */
int stack_pop() {
    if (thread_path_stack.top < 0) {
        strcpy(cur_v_path, "/"); // 栈已为空，重置路径为根目录
        thread_path_stack.top = -1; // 确保栈顶为 -1
        return -1; // 表示栈已为空
    }

    // 释放栈顶目录名的内存并弹出
    free(thread_path_stack.dir[thread_path_stack.top]);
    thread_path_stack.dir[thread_path_stack.top--] = NULL;

    // 重建虚拟路径
    if (thread_path_stack.top >= 0) {
        char temp[PATH_MAX] = "/"; // 从根目录开始重建
        for (int i = 0; i <= thread_path_stack.top; i++) {
            strncat(temp, thread_path_stack.dir[i], PATH_MAX - strlen(temp) - 1);
            if (i < thread_path_stack.top) {
                strncat(temp, "/", PATH_MAX - strlen(temp) - 1);
            }
        }
        strncpy(cur_v_path, temp, PATH_MAX);
        cur_v_path[PATH_MAX - 1] = '\0'; // 确保路径以 null 结尾
    } else {
        strcpy(cur_v_path, "/"); // 栈为空，路径为根目录
    }

    return 0; // 返回成功
}

/**
 * @brief 清空目录栈，释放所有目录名称所占内存
 * @return 0 表示成功，-1 表示失败（当前未定义具体失败场景）
 * @note 重置栈为初始状态，栈顶指针设为 -1，虚拟路径设为 "/"
 * @warning 使用 _Thread_local 变量确保线程安全
 */
int stack_clear() {
    while (thread_path_stack.top >= 0) {
        free(thread_path_stack.dir[thread_path_stack.top]);
        thread_path_stack.dir[thread_path_stack.top--] = NULL;
    }
    current_dir_id = 0; // 重置目录 ID 为根目录
    strcpy(cur_v_path, "/"); // 重置路径为根目录
    return 0; // 返回成功
}

/**
 * @brief 切换目录
 * @param mysql 数据库连接
 * @param path 目标路径字符串
 * @param response 写入响应消息的缓冲区
 * @param res_size 响应缓冲区的大小
 * @return SUCCESS 表示成功，特定错误代码表示失败
 */
int dir_cd(MYSQL *mysql, const char *path, char *response, size_t res_size) {
    printf("[调试] 进入 dir_cd()，输入路径为：'%s'\n", path);

    if (!mysql || !path || !response || res_size == 0) {
        snprintf(response, res_size, "cd: 内部错误 - 无效参数。\n");
        return ERR_PARAM;
    }

    if (path[0] == '\0') {
        snprintf(response, res_size, "当前目录：%s （ID : %d）\n", cur_v_path, current_dir_id);
        return SUCCESS;
    }

    char tmp_path[PATH_MAX];
    strncpy(tmp_path, path, PATH_MAX - 1);
    tmp_path[PATH_MAX - 1] = '\0';

    int is_absolute = (tmp_path[0] == '/');
    if (is_absolute) {
        printf("[调试] 绝对路径，清空目录栈。\n");
        stack_clear();
    }

    char *token = strtok(tmp_path + (is_absolute ? 1 : 0), "/");
    while (token != NULL) {
        printf("[调试] 处理路径段：'%s'\n", token);

        if (strcmp(token, "..") == 0) {
            printf("[调试] 尝试退回上级目录。\n");
            if (stack_pop(mysql) == 0) {
                printf("[调试] 成功弹出目录栈，查询上级目录 ID。\n");

                if (current_dir_id == 0) {
                    printf("[调试] 当前为根目录，无需更改。\n");
                } else {
                    char sql_parent[1024];
                    snprintf(sql_parent, sizeof(sql_parent),
                             "SELECT parent_id FROM file_info WHERE id = %d AND hash IS NULL AND type = 1",
                             current_dir_id);
                    printf("[调试] 执行 SQL 查询上级目录：%s\n", sql_parent);

                    MYSQL_RES *res_parent = NULL;
                    if (db_query(mysql, sql_parent, &res_parent) != SUCCESS || !res_parent || mysql_num_rows(res_parent) == 0) {
                        snprintf(response, res_size, "cd: 内部错误 - 无法找到父目录 ID。\n");
                        printf("[错误] 查询父目录失败。\n");
                        if (res_parent) mysql_free_result(res_parent);
                        stack_clear();
                        return FAILURE;
                    }
                    MYSQL_ROW row_parent = mysql_fetch_row(res_parent);
                    current_dir_id = atoi(row_parent[0]);
                    mysql_free_result(res_parent);
                    printf("[调试] 当前目录 ID 更新为：%d\n", current_dir_id);
                }
            } else {
                current_dir_id = 0;
                printf("[调试] 目录栈已空，重置为根目录。\n");
            }

        } else if (strcmp(token, ".") == 0) {
            printf("[调试] 遇到 '.'，保持在当前目录。\n");
        } else {
            printf("[调试] 查询子目录 '%s'\n", token);
            char sql_child[1024];
            snprintf(sql_child, sizeof(sql_child),
                     "SELECT id FROM file_info WHERE parent_id = %d AND filename = '%s' AND hash IS NULL AND type = 1",
                     current_dir_id, token);
            printf("[调试] 执行 SQL：%s\n", sql_child);

            MYSQL_RES *res_child = NULL;
            if (db_query(mysql, sql_child, &res_child) != SUCCESS || !res_child || mysql_num_rows(res_child) == 0) {
                snprintf(response, res_size, "cd: '%s': 没有这个目录\n", token);
                printf("[错误] 子目录 '%s' 不存在。\n", token);
                if (res_child) mysql_free_result(res_child);
                return FAILURE;
            }
            MYSQL_ROW row_child = mysql_fetch_row(res_child);
            int child_dir_id = atoi(row_child[0]);
            mysql_free_result(res_child);
            printf("[调试] 获取到子目录 ID：%d\n", child_dir_id);

            if (stack_push(token) != 0) {
                snprintf(response, res_size, "cd: '%s': 目录栈溢出\n", token);
                printf("[错误] 栈溢出，stack_push 失败。\n");
                return FAILURE;
            }

            current_dir_id = child_dir_id;
            printf("[调试] 更新当前目录 ID 为：%d\n", current_dir_id);
        }

        token = strtok(NULL, "/");
    }

    snprintf(response, res_size, "当前目录：%s （ID : %d）\n", cur_v_path, current_dir_id);
    printf("[调试] 路径切换完成，当前目录为 %s，ID 为 %d\n", cur_v_path, current_dir_id);
    return SUCCESS;
}