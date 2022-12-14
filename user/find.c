#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

/* 参照ls源文件完成 */
char*
fmtname(char *path)
{
    char *p;
    // Find first character after last slash.
    for(p=path+strlen(path); p >= path && *p != '/'; p--)
        ;
    p++;
    return p;
}

void
find(char *path, char *target)
{
    char buf[512], *p;
    int fd;
    // 声明与文件相关的结构体
    struct dirent de;
    struct stat st;

    if((fd = open(path, 0)) < 0){
        fprintf(2, "find: cannot open %s\n", path);
        return;
    }

    if(fstat(fd, &st) < 0){
        fprintf(2, "find: cannot stat %s\n", path);
        close(fd);
        return;
    }

    switch(st.type){
    case T_FILE:
        if ( strcmp(fmtname(path),target) == 0 ) { // target目标文件匹
            printf("%s\n",path);
        }
        break;

    case T_DIR:
        if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf) {
            printf("find: path too long\n");
            break;
        }

        strcpy(buf, path);
        p = buf+strlen(buf);
        *p++ = '/';

        while(read(fd, &de, sizeof(de)) == sizeof(de)){
            if(de.inum == 0 || strcmp(de.name,".") == 0 || strcmp(de.name,"..")==0)
                continue;
            memmove(p, de.name, DIRSIZ);
            p[DIRSIZ] = 0;
            if(stat(buf, &st) < 0){
                printf("find: cannot stat %s\n", buf);
                continue;
            }
            find(buf,target);  //递归查找
        }
        break;
    }
    close(fd);
}

int
main(int argc, char *argv[])
{
    if(argc < 2){
        printf("find needs more than three arguments!");
        exit(-1);
    }
    
    find(argv[1],argv[2]);
    
    exit(0);
}
