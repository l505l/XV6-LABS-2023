#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"

char*
fmtname(char *path)
{
  static char buf[DIRSIZ+1];
  char *p;

  // Find first character after last slash.
  for(p=path+strlen(path); p >= path && *p != '/'; p--)
    ;
  p++;

  // Return blank-padded name.
  if(strlen(p) >= DIRSIZ)
    return p;
  memmove(buf, p, strlen(p));
  memset(buf+strlen(p), 0, DIRSIZ-strlen(p));
  return buf;
}

int is_recurse(char *path)
{
    //printf("path=%s\n",path);
    char *buf=fmtname(path);
    //printf("buf=%s\n",buf);
    if(buf[0]=='.' && buf[1]==0)
    {
        return 0;
    } //.情况
    if(buf[0]=='.' && buf[1]=='.' && buf[2]==0)
    {
        return 0;
    }//..情况
    return 1;
} 

void
find (char *path,char *target)
{
  char buf[512], *p;
  int fd;
  struct dirent de;
  struct stat st;

  if((fd = open(path, O_RDONLY)) < 0){
    fprintf(2, "find: cannot open %s\n", path);
    return;
  }

  if(fstat(fd, &st) < 0){
    fprintf(2, "find: cannot stat %s\n", path);
    close(fd);
    return;
  }

  if(strcmp(fmtname(path),target)==0)
  {
    printf("%s\n",path);
  }

  switch(st.type){
  case T_FILE:
    //printf("%s %d %d %l\n", path, st.type, st.ino, st.size);
    break;

  case T_DIR:
    if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){
      printf("find: path too long\n");
      break;
    }
    strcpy(buf, path);
    p = buf+strlen(buf);
    *p++ = '/';
    while(read(fd, &de, sizeof(de)) == sizeof(de)){
      if(de.inum == 0)
        continue;
      memmove(p, de.name, DIRSIZ);
      p[DIRSIZ] = 0;
      if(stat(buf, &st) < 0){
        printf("find: cannot stat %s\n", buf);
        continue;
      }
      //printf("fmtname(buf):{%s} target:{%s}\n",fmtname(buf),target);
      //printf("fmtname(buf):{%s} target:{%s} cmp:%d\n",fmtname(buf),target,strcmp(fmtname(buf),target));

      //printf("%s\n",buf);
      //printf("%s %d %d %d\n", buf, st.type, st.ino, st.size);
      if(is_recurse(buf))
      {
        find(buf,target);
      }
    }
    break;
  }
  close(fd);
}

int
main(int argc, char *argv[])
{

  if(argc == 1)
  {
    printf("usage: find [path] [target]\n");
  }
  if(argc == 2){
    find(".",argv[1]);
  }
  if(argc ==3)
  {
    find(argv[1],argv[2]);
  }
  exit(0);
}
