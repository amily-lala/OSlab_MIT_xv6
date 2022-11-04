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

struct {
  struct spinlock lock;   // 避免出现死锁？ bucket[i]->bucket[j] bucket[j]->bucket[i]
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.

  struct buf head[NBUCKETS]; // 每个桶有对应的锁
  struct spinlock hashLock[NBUCKETS];
} bcache;

// LRU
struct buf *find_least_recent_used_buffer() {
  int idx = -1;
  uint min_ticks = 0xffffffff;
  for (int i = 0; i < NBUF; i++) {
    // 该buffer不属于任何桶
    if ((bcache.buf+i)->bucketNum == -1) return &bcache.buf[i];
    // 否则从其他桶中寻找未被引用且least_recent_used
    // bcache.buf[i].refcnt
    if ((bcache.buf+i)->refcnt == 0 && (bcache.buf+i)->recent_used_time < min_ticks) {
      // 更新最小值记录
      idx = i;
      min_ticks = (bcache.buf+i)->recent_used_time;
    }
  }

  // 返回LRU，不存在返回-1；
  if (idx == -1) return 0;
  return (&bcache.buf[idx]);
}


// 用头插法向双向链表中加入buf
void add_buf(struct buf *head,struct buf *b) {
  b->prev = head;
  b->next = head->next;
  head->next->prev = b;
  head->next = b;
}

// 从双向链表中删掉buf
void delete_buf(struct buf *b) {
  b->next->prev = b->prev;
  b->prev->next = b->next;
}

void
binit(void)
{
  struct buf *b;

  initlock(&bcache.lock, "bcache");

  char buf[9];
  for (int i = 0; i < NBUCKETS; i++) {
    snprintf(buf,9,"bcache%d",i); // ???
    initlock(&bcache.hashLock[i],"bcache");
    // Create linked list of buffers
    bcache.head[i].prev = &bcache.head[i];
    bcache.head[i].next = &bcache.head[i];
  }
 
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    initsleeplock(&b->lock, "buffer"); // 初始化buffer的睡眠锁
    b->bucketNum = -1; // 初始化buffer所属桶号
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;

  int hashBucketNum = blockno%NBUCKETS; // 哈希映射得到的桶号

  acquire(&bcache.hashLock[hashBucketNum]);

  // Is the block already cached?
  for(b = bcache.head[hashBucketNum].next; b != &bcache.head[hashBucketNum]; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.hashLock[hashBucketNum]);
      acquiresleep(&b->lock);
      return b;
    }
  }

  release(&bcache.hashLock[hashBucketNum]);

  // Not cached.
  // 此时可能出现 bucket[i]->bucket[j] bucket[j]->bucket[i]死锁情况
  // 可能出现一个磁盘块被多次缓存
  acquire(&bcache.lock); // 一个block只能有一个缓存
  acquire(&bcache.hashLock[hashBucketNum]);
  
  // 防止在释放hashBucketNum锁的间隙被其他进程写入磁盘内容，故再次检验
  for(b = bcache.head[hashBucketNum].next; b != &bcache.head[hashBucketNum]; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.hashLock[hashBucketNum]);
      release(&bcache.lock);
      acquiresleep(&b->lock);
      return b;
    }
  }
  
  while (1) {
    // 找到一个least_recent_used_buffer
    // buffer来源可能有：own / never used / from other bucket / can't find
    b = find_least_recent_used_buffer();  
    if (b==0) continue;
    int old_idx = b->bucketNum;
    // 如果b不属于任何桶或者属于当前桶
    if (old_idx == -1 || old_idx == hashBucketNum) {
      b->dev = dev;
      b->blockno = blockno;
      b->valid = 0;
      b->refcnt = 1;
      b->bucketNum = hashBucketNum;

      // 如果b不属于任何桶，需要用头插法将b加入当前哈希桶的双向链表中 
      if (old_idx == -1) {
        add_buf(&bcache.head[hashBucketNum],b);
      }

      release(&bcache.hashLock[hashBucketNum]);
      release(&bcache.lock);
      acquiresleep(&b->lock);
      return b;
    } else {
      // b是从其他桶中窃取
      acquire(&bcache.hashLock[old_idx]);
      if (b->refcnt != 0 ) {
        release(&bcache.hashLock[old_idx]);
        continue;
      }
      b->dev = dev;
      b->blockno = blockno;
      b->valid = 0;
      b->refcnt = 1;
      b->bucketNum = hashBucketNum;

      // 原桶中删掉b
      delete_buf(b);
      // 加入当前哈希桶
      add_buf(&bcache.head[hashBucketNum],b);

      release(&bcache.hashLock[old_idx]);
      release(&bcache.hashLock[hashBucketNum]);
      release(&bcache.lock);
      acquiresleep(&b->lock);
      return b;
    }
  }

  release(&bcache.hashLock[hashBucketNum]);
  release(&bcache.lock);
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

  releasesleep(&b->lock);

  int hashTableNum = (b->blockno)%NBUCKETS;
  acquire(&bcache.hashLock[hashTableNum]);
  b->refcnt--;
  if (b->refcnt == 0) {
    // no one is waiting for it.
    // 这里因为有一个find_least_recent_used_buffer函数，所以不需要将缓冲从当前桶中释放，反正也是可以抢的吗
    b->recent_used_time = ticks;
  }
  release(&bcache.hashLock[hashTableNum]);
}

void
bpin(struct buf *b) {
  int hashTableNum = (b->blockno)%NBUCKETS;
  acquire(&bcache.hashLock[hashTableNum]);
  b->refcnt++;
  release(&bcache.hashLock[hashTableNum]);
}

void
bunpin(struct buf *b) {
  int hashTableNum = (b->blockno)%NBUCKETS;
  acquire(&bcache.hashLock[hashTableNum]);
  b->refcnt--;
  release(&bcache.hashLock[hashTableNum]);
}