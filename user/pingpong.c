#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "user/user.h"
#define MSGSIZE 16 
char* msg1="ping";
char* msg2="pong";


int
main(int argc, char *argv[])
{
    char inbuf[MSGSIZE];
    int p[2],pid;
    if(pipe(p)<0)
        exit(1);
    if((pid=fork())>0){
        write(p[1],msg1,MSGSIZE);
        read(p[0],inbuf,MSGSIZE);
        printf("%d: received %s\n",getpid(),inbuf);
    }
    else{
        read(p[0],inbuf,MSGSIZE);
        printf("%d: received %s\n",getpid(),inbuf);
        write(p[1],msg2,MSGSIZE);
    }
    exit(0);
}