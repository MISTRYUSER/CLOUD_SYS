#include <my_header.h>
#include "fileserver.h"
#include <stdio.h>
#include <string.h>

// 测试函数
void test_fileserver() {
    // 测试日志
    log_message(LOG_INFO,  "测试日志: 这是 info 级别");
    log_message(LOG_WARN,  "测试日志: 这是 warn 级别");
    log_message(LOG_ERROR, "测试日志: 这是 error 级别");
    log_message(LOG_INFO,  "测试日志: 只写一行字符串");

    // 测试进度保存与读取
    const char* testfile = "testfile.data";
    save_upload_progress(testfile, 12345);
    long up = read_upload_progress(testfile);
    log_message(LOG_INFO, "读取上传进度: %ld (应为12345)", up);

    save_download_progress(testfile, 54321);
    long down = read_download_progress(testfile);
    log_message(LOG_INFO, "读取下载进度: %ld (应为54321)", down);

    // 测试分片上传与下载模拟
    char chunk[] = "hello chunk!";
    int fake_sock = -1; // 假设socket无效，只测试接口，不实际传输
    int result = upload_chunk(fake_sock, chunk, strlen(chunk));
    log_message(LOG_INFO, "调用 upload_chunk 返回值: %d (应为-1，因socket无效)", result);

    // 可以继续模拟其它接口的调用...

    printf("fileserver test done! 请查看 fileserver.log 日志文件。\n");
}

// main主函数（调试时可用）
#ifdef TEST_FILESERVER_MAIN
int main() {
    test_fileserver();
    return 0;
}
#endif
