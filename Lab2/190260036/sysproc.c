#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "processInfo.h"

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

// ###########################################
// Additions to this file start below.

// prints Hello to the console.
int
sys_hello(void)
{
  cprintf("%s\n", "Hello");
  return 0;
}

// prints passed string to the console.
int 
sys_helloYou(void)
{
  char *str;

  begin_op();
  if(argstr(0, &str) < 0){
    end_op();
    return -1;
  }
  cprintf("%s\n", str);
  end_op();
  return 0;
}

// Returns the total number of active processes
// in the system (either in embryo, running,
// runnable, sleeping, or zombie states).
int 
sys_getNumProc(void)
{
  return numProc(); // function in proc.c
}

// Returns the maximum PID amongst the PIDs
// of all currently active processes in the
// system.
int 
sys_getMaxPid(void)
{
  return maxPid(); // function in proc.c
}

// This system call takes as arguments an integer 
// PID and a pointer to a structure processInfo. 
// This structure is used for passing information 
// between user and kernel mode.
int
sys_getProcInfo(void)
{
  int pid;
  struct processInfo* procInfo;
  
  if (argint(0, &pid) < 0 || argptr(1, (void*)&procInfo, sizeof(*procInfo)) < 0)
    return -1;

  procInfoRet(pid, procInfo); // function in proc.c
  return 0;
}

// This system call takes the address of the 
// welcome function as an argument. If this 
// welcome function is not set, child processes 
// should begin execution right after fork, as 
// usual. If a welcome function address is set 
// using this system call, new processes should 
// begin execution in this welcome function 
// instead.
int
sys_welcomeFunction(void)
{
  void(*welcomeFuncPtr)(void);

  if (argptr(0, (void*)&welcomeFuncPtr, sizeof(*welcomeFuncPtr)) < 0)
    return -1;

  setWelcomeFunction(welcomeFuncPtr); // function in proc.c
  return 0;
}


// The child must invoke the system call 
// welcomeDone to go back to executing code after 
// the fork system call, like regular child 
// processes do. This system call takes no 
// arguments, and will be invoked by the child  at 
// the end of its execution of the welcome function.
int
sys_welcomeDone(void)
{
  setWelcomeDone(); // function in proc.c
  return 0;
}