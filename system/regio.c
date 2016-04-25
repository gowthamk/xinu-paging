#include<xinu.h>

unsigned long tmp;

void write_cr0(unsigned long n) {
  intmask ps = disable();
  tmp = n;
  asm("pushl %eax");
  asm("movl tmp, %eax");        
  asm("movl %eax, %cr0");
  asm("popl %eax");
  restore(ps);
}
unsigned long read_cr0(void) {
  unsigned long local_tmp;
  intmask ps;
  ps = disable();
  asm("pushl %eax");
  asm("movl %cr0, %eax");
  asm("movl %eax, tmp");
  asm("popl %eax");
  local_tmp = tmp;
  restore(ps);
  return local_tmp;
}

void write_cr3(unsigned long n) {
  kprintf("Writing 0x%08X into CR3\n",n);
  intmask ps;
  ps = disable();
  tmp = n;
  asm("pushl %eax");
  asm("movl tmp, %eax");                
  asm("movl %eax, %cr3");
  asm("popl %eax");
  restore(ps);
}

unsigned long read_cr3(void) {
  unsigned long local_tmp;
  intmask ps;
  ps = disable();
  asm("pushl %eax");
  asm("movl %cr3, %eax");
  asm("movl %eax, tmp");
  asm("popl %eax");
  local_tmp = tmp;
  restore(ps);
  return local_tmp;
}

void write_cr2(unsigned long n) {
  intmask ps;
  ps = disable();
  tmp = n;
  asm("pushl %eax");
  asm("movl tmp, %eax");                
  asm("movl %eax, %cr2");
  asm("popl %eax");
  restore(ps);
}

unsigned long read_cr2(void) {
  unsigned long local_tmp;
  intmask ps;
  ps = disable();
  asm("pushl %eax");
  asm("movl %cr2, %eax");
  asm("movl %eax, tmp");
  asm("popl %eax");
  local_tmp = tmp;
  restore(ps);
  return local_tmp;
}

