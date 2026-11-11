#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "vm.h"
#include "sysinfo.h"
uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  kexit(n);
  return 0; // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return kfork();
}

uint64
sys_wait(void)
{
  uint64 p;
  argaddr(0, &p);
  return kwait(p);
}

uint64
sys_sbrk(void)
{
  uint64 addr;
  int t;
  int n;

  argint(0, &n);
  argint(1, &t);
  addr = myproc()->sz;

  if (t == SBRK_EAGER || n < 0) {
    if (growproc(n) < 0) {
      return -1;
    }
  } else {
    // Lazily allocate memory for this process: increase its memory
    // size but don't allocate memory. If the processes uses the
    // memory, vmfault() will allocate it.
    if (addr + n < addr)
      return -1;
    if (addr + n > TRAPFRAME)
      return -1;
    myproc()->sz += n;
  }
  return addr;
}

uint64
sys_pause(void)
{
  int n;
  uint ticks0;

  argint(0, &n);
  if (n < 0)
    n = 0;
  acquire(&tickslock);
  ticks0 = ticks;
  while (ticks - ticks0 < n) {
    if (killed(myproc())) {
      release(&tickslock);
      return -1;
    }
    sleep_prepare(&ticks);
    release(&tickslock);
    sleep();
    acquire(&tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  argint(0, &pid);
  return kkill(pid);
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

uint64
sys_hello(void)
{
  struct proc *p = myproc();

  printk("kernel: hello() called by pid %d (running in kernel)\n",
         p->pid); //do trong defs.h dung printk, khong co printf

  return 0;
}

uint64
sys_cont_create(void)
{
  int max_proc;

  argint(0, &max_proc);

  return kcontainer_create(max_proc);
}

uint64
sys_cont_enter(void)
{
  int id;

  argint(0, &id);

  return kcontainer_enter(id);
}

uint64
sys_cont_info(void)
{
  int id;
  uint64 addr;
  struct proc *p;
  struct container_info info;

  argint(0, &id);
  argaddr(1, &addr);

  if (kcontainer_info(id, &info) < 0)
    return -1;

  p = myproc();

  if (copyout(p->pagetable, p->sz, addr, (char *)&info, sizeof(info)) < 0)
    return -1;

  return 0;
}

uint64
sys_cont_set_cpu(void)
{
  if (!container_is_host())
    return -1;
  return -1;
}

uint64
sys_cont_set_mem(void)
{
  if (!container_is_host())
    return -1;
  return -1;
}

uint64
sys_sysinfo(void)
{
  uint64 addr;
  struct proc *p = myproc();
  struct sysinfo info;

  argaddr(0, &addr);

  info.freemem = kfreemem();
  info.nproc = knproc();

  if (copyout(p->pagetable, p->sz, addr,
              (char *)&info, sizeof(info)) < 0)
    return -1;

  return 0;
}
