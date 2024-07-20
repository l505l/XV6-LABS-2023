#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "user/user.h"
#define MSGSIZE 1
#define START_NUMBER 2
#define END_NUMBER 35

void check_number(int *fd)
{
    close(fd[1]);
    int prime=0;
    read(fd[0],&prime,MSGSIZE);
    printf("prime %d\n",prime);
    
    int next_num=0;
    if(read(fd[0],&next_num,MSGSIZE)==0) //到达文件末尾
        return;
    //继续创建子进程,新管道
    int new_fd[2];
    pipe(new_fd);
    int pid=fork();
    if(pid>0)
    {
        //父进程筛选素数
        //关闭读
        close(new_fd[0]);
        do{
            if(next_num % prime !=0) //是素数 
            {
                //通过新管道写给新的子进程
                write(new_fd[1],&next_num,MSGSIZE);
            }
        }while(read(fd[0],&next_num,MSGSIZE));
        close(fd[0]);
        close(new_fd[1]);
        //等待一段时间
        int status;
        wait(&status);
    }
    else{
        close(fd[0]);
        //子进程重复检查工作
        check_number(new_fd);
    }

}

int
main(int argc, char *argv[])
{
    if(argc !=1)
    {
        printf("Wrong comment!\n");
        exit(1);
    }

    int fd[2];
    pipe(fd);
    int pid=fork();
    if(pid>0)
    {
        //父进程
        close(fd[0]);
        for(int i=START_NUMBER;i<=END_NUMBER;i++)
        {
            write(fd[1],&i,MSGSIZE);
        }
        //关闭写通道
        close(fd[1]);
        int status;
        wait(&status);
    }
    else{
        //子进程
        check_number(fd);
    }
    exit(0);
}