// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct kmem{
  struct spinlock lock; // multi process compete for this lock
  struct run *freelist; // free physical page : 1.0 4kB/page ; 2.0 spinlock
};

// lab 3:
// 实验思路：为每个CPU核分配独立的内存链表
// 数据结构：kmems 是一个kmem数组，数组容量为NCPU (max)
// TODO:
// 1.0 修改相关和内存管理器有关的操作
// 2.0 优化，减少锁争用
struct kmem kmems[NCPU];

void
kinit()
{
  // TODO:STEP1 initlock
  for (int i = 0; i < NCPU; i++) {
    initlock(&kmems[i].lock, "kmem"+i);
  }
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  // TODO :STEP3 优先将内存放入当前CPU的freelist中
  push_off(); // 关中断
  int cpuID = cpuid(); // 获取当前CPU的id
  acquire(&kmems[cpuID].lock);
  r->next = kmems[cpuID].freelist;
  kmems[cpuID].freelist = r;
  release(&kmems[cpuID].lock);
  pop_off(); // 开中断

}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  // TODO:STEP2 优先为当前CPU分配内存

  push_off(); // 关中断

  int cpuID = cpuid(); // 获取当前CPU的id
  acquire(&kmems[cpuID].lock);
  r = kmems[cpuID].freelist;
  if(r) {
    kmems[cpuID].freelist = r->next;
  }
  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  release(&kmems[cpuID].lock);

  pop_off(); // 开中断  // 位置？？？？
  return (void*)r;

  // TODO:若当前CPU没有空闲内存块，则从其他CPU的freelist窃取
  // Q:按照for循环的顺序，还是随机一点好呢？
  if(!r) {
    for (int i = 0; i < NCPU; i++) {
      if (i != cpuID) {
        acquire(&kmems[i].lock);
        r = kmems[i].freelist;
        if(r) {
          kmems[cpuID].freelist = r->next;
        }
        if(r)
          memset((char*)r, 5, PGSIZE); // fill with junk
        release(&kmems[i].lock);
        return (void*)r;
      }
    }
  }

  return 0; //没有空闲内存，返回0（null）
}
 