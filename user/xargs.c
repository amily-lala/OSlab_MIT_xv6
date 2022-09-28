#include <stddef.h>
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

/**
  * 解析：| 前面的内容可直接从标准输入读入，即管道0 ； | 后面的内容是main函数中传入的参数 argv[] ; xargs 不仅需要处理stdin 还需要
 * 思路：
 */

#define MAXN 1024
#define MAXARG 512

int
main(int argc, char *argv[])
{
    // //检查参数数量是否正确
    // if (argc < 2) {
    //     print("xargs:needs more than 3 arguments!");
    //     exit(-1);
    // }

    char all_buf[MAXARG];
    char *buf = all_buf;
    char *pi;
    pi = buf;

    // 存放子进程 exec 的参数
    char *argvs[MAXARG];

    int sum = 0; // 记录缓冲区字符串长度

    // 除去xargs,保留命令符，默认为echo (to do)
    int index = 0;
    if (argc == 1) {
        argvs[index++] = "echo";
    } else {
        for (int i = 1; i < argc; ++i) { // 除去xargs,保留命令符，默认为echo (to do)
            argvs[index++] = argv[i];
        }
    }

    int len;
    while((len = read(0, buf, MAXN)) > 0 ) {
        sum += len;
        if (sum > 256) {
            printf("xargs:the args is too long!\n");
        }
        for (int i = 0; i < len; i++) { 
            if (buf[i] == ' ' ) {    // 读到完整的一个参数
                buf[i] = '\0';
                argvs[index++] = pi;    // 加入参数
                int j = i+1;
                pi = &buf[j]; 
            } else if (buf[i] == '\n') {
                buf[i] = '\0';
                argvs[index++] = pi;    // 加入参数
                int j = i+1;
                pi = &buf[j];
            }
        }
    }
    
    int ret = fork();
    if (ret < 0) {
        printf("fork failed!");
    } else if (fork() == 0) {
        exec(argv[1],argvs);
        exit(0);
    } else {
        wait(0);
    }
    exit(0);
}

