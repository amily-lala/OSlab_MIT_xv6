#include <stddef.h>
#include "kernel/param.h"
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define MAXN 512
/**
  * 解析：| 前面的内容可直接从标准输入读入，即管道0 ； | 后面的内容是main函数中传入的参数 argv[] ; xargs 不仅需要处理stdin 还需要
 * 思路：
 */

int
main(int argc, char *argv[])
{
    // 存放子进程 exec 的参数
    char *argvs[MAXARG];

    // 除去xargs,保留命令符，默认为echo (to do)
    char *cmd = "/bin/echo";
    int index = 0;
    if (argc == 1) {
        /* 无命令符*/
        argv[index++] = cmd; 
        argvs[1] = "echo";
        ++argc;
    } else {
        for (int i = 1; i < argc; ++i) { // 除去xargs,保留命令符，默认为echo (to do)
            argvs[index++] = argv[i];
        }
    }

    char all_buf[MAXN]; // 标准输入中读入的所有参数


    int sum = 0; // 记录缓冲区字符串长度
    int len;
    while((len = read(0, all_buf, MAXN)) > 0 ) {
        char *buf = all_buf;
        char *pi; // 单个参数
        pi = buf;
        sum += len;
        if (sum > 256) {
            printf("xargs:the args is too long!\n");
        }
        for (int i = 0; i < len; i++) { 
            if (buf[i] == ' ' ) {    // 读到完整的一个参数
                buf[i] = '\0';
                argvs[index++] = pi;  // 加入参数
                int j = i+1;
                pi = &buf[j]; // 指向下一个参数
            } else if (buf[i] == '\n') { //一个命令行
                buf[i] = '\0';
                argvs[index++] = pi;    // 加入参数
                int j = i + 1;
                if (fork() == 0) {
                    exec(argv[1],argvs);
                    // exit(0);
                } 
                wait(0);
                index = argc - 1; // 附加参数重置
                pi = &buf[j];
            } 
        }
    }
    exit(0);
}

