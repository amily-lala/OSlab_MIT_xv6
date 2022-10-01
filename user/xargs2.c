#include "kernel/types.h"
#include "kernel/param.h"
#include "user/user.h"

int main(int argc,char *argv[])
{
    char line[1024];
    char* params[MAXARG];
    int index = 0;
    for(int i=1;i<argc;i++) params[index++]=argv[i];
    int n = 0;
    char *cmd = argv[1];
    while((n=read(0,line,1024))>0)
    {
        if(fork()==0)
        {
            char *arg = (char *)malloc(sizeof(line));
            int arg_index = 0;
             for(int i=0;i<n;i++)
             {
                 if(line[i]==' '||line[i]=='\n')
                 {
                     arg[arg_index]=0;
                     arg_index = 0;
                     params[index++]=arg;
                     //free(arg);
                     arg = (char *)malloc(sizeof(line));
                 }
                 else
                 {
                     arg[arg_index++]=line[i];
                 }

             }
             params[index] = 0;
             exec(cmd,params);   
        }
        else
        {
            wait(0);
        }
    }
    return 0;
}
