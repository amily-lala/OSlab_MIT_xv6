#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "sysinfo.h"

uint64
sys_exit(void)
{
  int n;
  if(argint(0, &n) < 0)
    return -1;
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  if(argaddr(0, &p) < 0)
    return -1;
  return wait(p);
}

uint64
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}


// sys_trace 
// print info of traced syscall
// info form should be like    PID：sys_$name(arg0) -> return_value
uint64 
sys_trace(void)
{
  // 将a0寄存器中的返回值传递给现在进程的mask
  argint(0,&myproc()->mask);
  return 0;
}


// copy from kernel to user
// return process/freemem/freefd
uint64
sys_sysinfo(void) 
{
  struct proc *p = myproc();
  struct sysinfo sinfo;
  uint64 addr; // user pointer to strcut sysinfo

  if(argaddr(0,&addr) < 0){
    return -1;
  }

  // get num we need 1.freemem 2.nproc 3.freefd
  sinfo.freemem = cal_freeMem();
  sinfo.nproc = cal_nproc();
  sinfo.freefd = cal_freefd();

  // deliver to user
  if (copyout(p->pagetable,addr,(char *)&sinfo,sizeof(sinfo)) < 0) {
    return -1;
  }
  return 0;
}