#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/param.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"
#define MSGSIZE 16 


int
main(int argc, char *argv[])
{
    //由于find递归调用耗时较长，因此先等待
    sleep(10);
    char buf[MSGSIZE];
    //1、获取上一个命令的标准化输入
    read(0,buf,MSGSIZE);
    //printf("上一个命令的标准化输入为:%s\n",buf);  //调试信息


    //2、获取命令行命令的参数
    char * xargv[MAXARG];
    int xargc=0;
    for(int i=1;i<argc;i++)
    {
        xargv[xargc++]=argv[i];
    }

    char *p=buf;
    for(int i=0;i<MSGSIZE;i++)
    {
        if(buf[i]=='\n')
        {
            int pid=fork();
            if(pid>0)
            {
                //父进程
                p=&buf[i+1];  //继续读取下一个参数
                wait(0);
            }
            else
            {
                //子进程
                buf[i]=0; //结束标志符
                xargv[xargc]=p; //提取
                xargc++;   
                xargv[xargc]=0; //结束标识符
                xargc++;
                exec(xargv[0],xargv); //执行命令
                exit(1);
            }
        }
    }
    exit(0);
}