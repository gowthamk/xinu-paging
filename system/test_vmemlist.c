#include<xinu.h>

void print_vmemlist(struct vmemblk* cur) {
    kprintf("\n\n----------------------\n");
    while(cur != NULL) {
       kprintf("[0x%08X,0x%08X](%d)\t",cur->mbegin
            , (uint32)cur->mbegin + cur->mlength-1, cur->mlength);
        cur = cur->mnext;
    }
    kprintf("\n----------------------\n");
    return;
}

process test_vmemlist() {
    struct vmemblk *cur = proctab[currpid].prvmemlist;
    print_vmemlist(cur);
    char* p1 = vgetmem(200);
    print_vmemlist(cur);
    if(vfreemem(p1+20,10) == SYSERR) {
        kprintf("SYSERR\n");
    }
    print_vmemlist(cur);
    vfreemem(p1+60,5);
    print_vmemlist(cur);
    vfreemem(p1+30,30);
    print_vmemlist(cur);
    vfreemem(p1,10);
    print_vmemlist(cur);
    vfreemem(p1+10,10);
    print_vmemlist(cur);
    return OK;
}

struct about_me {
    pid32 pid;
    char msg[32];
};
void print_about(struct about_me* p) {
    kprintf("[Pid %d] My msg is %s\n",p->pid,p->msg);
    return;
}
process test_vmem_1() {
    kprintf("---------------------------------------------------------------\n");
    kprintf("Test1: User-driven eviction \n");
    kprintf("---------------------------------------------------------------\n");
    struct about_me* p = (struct about_me*)vgetmem(sizeof(struct about_me));
    p->pid = currpid;
    char* msg = "test_vmem_1";
    strncpy(p->msg,msg,strnlen(msg,100));
    print_about(p);
    print_ipt_stats();
    evict_vframe(vframe_of(p));
    print_ipt_stats();
    print_about(p);
    print_ipt_stats();
    //  vfreemem((char*)p1,200);
    //  evict_vframe(vframe_of(p1));
    //  print_ipt_stats();
    //  p1 = (int*)vgetmem(100);
    //  kprintf("The value at 0x%08X is %d\n",p1,*p1);
    //  kprintf("The value at 0x01000000 is %d\n",*((int*)0x01000000));
    //  print_ipt_stats();
    //  int* p2 = (int*) ((uint32)p1 + NBPG);
    //  kprintf("The value at 0x%08X is %d\n",p2,*p2);
    //  print_ipt_stats();
    return OK;
}
process test_vmem_2() {
    kprintf("---------------------------------------------------------------\n");
    kprintf("Test2: Automatic eviction \n");
    kprintf("---------------------------------------------------------------\n");
    print_ipt_stats();
    struct about_me *p1, *p2;
    char *msg1 = "Hello, World!", *msg2 = "Goodbye, World!";
    p1 = (struct about_me*)vgetmem(NBPG);
    p1->pid = currpid;
    strncpy(p1->msg,msg1,strnlen(msg1,100));
    print_ipt_stats();
    p2 = (struct about_me*)vgetmem(NBPG);
    p2->pid = currpid;
    strncpy(p2->msg,msg2,strnlen(msg2,100));
    print_ipt_stats();
    int i;
    for(i=0; i<5; i++) {
        print_about(p1);
        print_about(p2);
    }
    return OK;
}

process proc_vmem_3(char *msg,int t) {
    struct about_me* p = (struct about_me*)vgetmem(NBPG);
    p->pid = currpid;
    strncpy(p->msg,msg,strnlen(msg,100));
    print_ipt_stats();
    int i;
    for(i=0; i<5; i++) {
        sleepms(t);
        print_about(p);
    }
    return OK;
}

int test_vmem_3() {
    kprintf("---------------------------------------------------------------\n");
    kprintf("Test3: Two processes - automatic eviction \n");
    kprintf("---------------------------------------------------------------\n");
    print_ipt_stats();
    resume(vcreate(proc_vmem_3, 8192, INITHEAP, 50, "test31",2, "Hello, World!",500));
    resume(vcreate(proc_vmem_3, 8192, INITHEAP, 50, "test32",2, "Goodbye, World!",600));
    print_ipt_stats();
    return 0;
}
