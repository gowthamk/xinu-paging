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

process test_vmem() {
    int* p1 = (int*)vgetmem(200);
    *p1 = 20;
    kprintf("The value at 0x%08X is %d\n",p1,*p1);
    print_ipt_stats();
    vfreemem((char*)p1,200);
    evict_frame(vframe_of(p1));
    return OK;
}
