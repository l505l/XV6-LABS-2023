// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

#define NBUCKET 13  //定义桶的个数

int hash(int i)
{
  return i%NBUCKET;
} //哈希函数

struct {
  struct spinlock lock[NBUCKET];
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  struct buf head[NBUCKET];
} bcache;

void
binit(void)
{
  struct buf *b;
  char lock_name[16];
  for(int i=0;i<NBUCKET;i++)
  {
    snprintf(lock_name,sizeof(lock_name),"bucket_%d",i);
    initlock(&bcache.lock[i], lock_name);
    //初始化桶的头节点
    bcache.head[i].prev = &bcache.head[i];
    bcache.head[i].next = &bcache.head[i];
  }
    

  // Create linked list of buffers

  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    //头插法初始化缓冲区列表
    b->next = bcache.head[0].next;
    b->prev = &bcache.head[0];
    initsleeplock(&b->lock, "buffer");
    bcache.head[0].next->prev = b;
    bcache.head[0].next = b;
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;

  
  int id=hash(blockno);
  acquire(&bcache.lock[id]);
  // Is the block already cached?
  for(b = bcache.head[id].next; b != &bcache.head[id]; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;

      //获取时间并记录时间戳
      acquire(&tickslock);
      b->timestamp=ticks;
      release(&tickslock);


      release(&bcache.lock[id]);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached.
  b = 0;
  // Recycle the least recently used (LRU) unused buffer.
  for(int i=id,times=0;times!=NBUCKET;i=(i+1)%NBUCKET)
  {
    times++;
    if(i!=id)
    {
      if(!holding(&bcache.lock[i])) //未占用
      acquire(&bcache.lock[i]);
      else continue;
    }
    

    for(struct buf*new_b=bcache.head[id].next;new_b!=&bcache.head[id];new_b=new_b->next)
    {
      //通过时间戳来进行LRU算法查找
      if(new_b->refcnt==0 && (b==0 || new_b->timestamp < b->timestamp))
      {
        b=new_b;
      }
    }

    if(b)   //若不为空
    {
      if(i!=id)  //头插法插入当前桶
      {
        b->next->prev=b->prev;
        b->prev->next=b->next;
        release(&bcache.lock[i]);

        b->next=bcache.head[id].next;
        b->prev=&bcache.head[id];
        bcache.head[id].next->prev=b;
        bcache.head[id].next=b;
      }
      b->dev = dev;
      b->blockno = blockno;
        b->valid = 0;
        b->refcnt = 1;
        
        acquire(&tickslock);
        b->timestamp=ticks;
        release(&tickslock);

        release(&bcache.lock[id]);
        acquiresleep(&b->lock);
        return b;
    }
    else{
      //未找到则释放
      if(i !=id)
        release(&bcache.lock[i]);
    }
    
  } 
  panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  
  if(!holdingsleep(&b->lock))
    panic("brelse");
  int id=hash(b->blockno);//获取散列桶id
  releasesleep(&b->lock);

  acquire(&bcache.lock[id]);
  b->refcnt--;
  
  //更新时间
  acquire(&tickslock);
  b->timestamp=ticks;
  release(&tickslock);
  
  release(&bcache.lock[id]);
}

void
bpin(struct buf *b) {
  int id=hash(b->blockno);
  acquire(&bcache.lock[id]);
  b->refcnt++;
  release(&bcache.lock[id]);
}

void
bunpin(struct buf *b) {
  int id=hash(b->blockno);
  acquire(&bcache.lock[id]);
  b->refcnt--;
  release(&bcache.lock[id]);
}


