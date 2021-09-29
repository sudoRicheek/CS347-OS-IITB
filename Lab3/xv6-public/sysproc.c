#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"

int
sys_fork(void)
{
  return fork();
}

int
sys_exit(void)
{
  exit();
  return 0;  // not reached
}

int
sys_wait(void)
{
  return wait();
}

int
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

int
sys_getpid(void)
{
  return myproc()->pid;
}

int
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

int
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

// return how many clock tick interrupts have occurred
// since start.
int
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

// Returns the number of virtual/logical pages in the user part of the address space
// of the process, up to the program size stored in struct proc. Count of the stack guard
// pages are also accounted in the calculations.
int
sys_numvp(void)
{
  int numpages = help_numvp();
  return numpages;
}

// Should return the number of physical pages in the user part of the address space of the
// process. We count this number by walking the process page table, and counting the number
// of page table entries that have a valid physical address assigned.
int 
sys_numpp(void)
{
  int numphypages = help_numpp();
  return numphypages;
}

// Simplified version of the mmap function
// in xv6
int
sys_mmap(void)
{
  int numbytes;

  if (argint(0, &numbytes) < 0)
    return 0; // 0 for Failure

  return help_mmap(numbytes);
}