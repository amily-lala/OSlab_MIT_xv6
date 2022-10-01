#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

int main (int argc, char *argv[]) {
    int p1[2],p2[2]; // p1->parent p2->child
    pipe(p1);
    pipe(p2);

    char buf[5];
    char msg1[5] = "ping"; // msg:parent-->son
    char msg2[5] = "pong"; // mag:son-->parent

    int ret = fork(); //产生子进程
   
    if (ret < 0) {
        /*子进程产生失败*/
        printf("fork failed!");
        exit(-1);
    } else if (ret == 0) {
        /* child */
        close(p1[1]);//关闭父进程的写端
        close(p2[0]);//关闭子进程的读端
        read(p1[0],buf,sizeof(buf)); //打开父进程的读端
        fprintf(1,"%d: received %s\n",getpid(),buf);
        close(p1[0]);  //关闭父进程的读端
        write(p2[1],msg2,strlen(msg2)); //打开子进程的写端
        close(p2[1]);  //关闭子进程的写端
    } else {
        /* parent */
        close(p1[0]);//关闭父进程读端
        close(p2[1]);//关闭子进程写端
        write(p1[1],msg1,sizeof(msg1));
        close(p1[1]);
        read(p2[0],buf,sizeof(buf));//打开子进程的读端
        fprintf(1,"%d: received %s\n",getpid(),buf);
        close(p2[0]);//关闭子进程读端
        wait(0);//等待子进程结束
    }
    exit(0);
}