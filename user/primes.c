#include "kernel/types.h"
#include "user/user.h"

int main(int argc, char *argv[]) {
    int p1[2];
    pipe(p1);

    /* 将待处理的数写入管道 */
    int num;
    for (num = 2; num <= 35; num++) {
        write(p1[1], &num, sizeof(num));
    }
    num = 0;
    write(p1[1], &num, sizeof(num)); // 0作为读取结束标志

    while (1) {
        int p2[2];
        pipe(p2);
        int num1, prime;
        read(p1[0], &prime, sizeof(prime)); //"父"进程从管道中读取到的第一个数是质数
        printf("prime %d\n", prime);
        if (read(p1[0], &num1, sizeof(num1)) && num1) {
            int ret = fork();
            if (ret < 0) {
                /* fork failed*/
                printf("fork failed!");
                exit(-1);
            } else if (fork() == 0) {
                /* child*/
                close(p1[0]);
                close(p1[1]);
                p1[0] = p2[0];
                p1[1] = p2[1];
                continue;
            } else {
                /* parent */
                do {
                    if (num1 % prime != 0)
                    write(p2[1], &num1, sizeof(num1));
                } while (read(p1[0], &num1, sizeof(num1)) && num1);
                write(p2[1], &num1, sizeof(num1)); // 筛选掉当前质数的倍数 
            }
        }
        // 关闭占用的管道
        close(p1[0]);
        close(p1[1]);
        close(p2[0]);
        close(p2[1]);
        wait(0);
        break;
    }
    exit(0);
}
