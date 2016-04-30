/* pfhandler.c - pfhandler */

#include<xinu.h>
long pferrcode;
extern char* pf_test_ptr;
uint32 pfla, last_pfla = 0;
/*------------------------------------------------------------------------
 * pfhandler - high level page fault handler
 *------------------------------------------------------------------------
 */
status fill_pt_entry(pt_t* pt_entry) {
    /* NOTE: It is important to increment reference count before calling
     * getframe. Otherwise, if getframe evicts a frame, it may free up the
     * current page table!*/
    frame_ref_inc((uint32)pt_entry);
    int vfno = vframe_of(pfla);
    /* increment reference count for current pt */
    char* newpg = getframe(DEFAULT_FRAME,currpid,vfno);
    if(newpg == NULL) {
        kprintf("Insufficient Memory!\n");
        kprintf("Killing pid %d.\n",currpid);
        kill(currpid);
        return SYSERR;
    }
    /* Copy virtual frame from bs, if one exists */
    bsd_t bs = proctab[currpid].prbs;
    if(bs == -1) {
        kprintf("Invalid backing store for pid %d.\n",currpid);
        return SYSERR;
    }
    int retries = 4, status;
    //kprintf("Reading vfno %d from bs... ",vfno);
    while((status = read_bs(newpg,bs,vfno)) != OK  && retries > 0) {
        retries --;
    }
    if(status != OK) {
        //kprintf("FAILED!\n");
        /* Alas, life has to move on. */
    } else {
        //kprintf("OK\n");
    }

    /* Add new page's address to pt_entry */
    pt_entry->pt_pres = 1;
    pt_entry->pt_base = (uint32) newpg >> 12;
    //kprintf("fill_pt_entry filled pt_entry at 0x%08X as 0x%08X\n",pt_entry,*pt_entry);
    return OK;
}
status fill_pd_entry(pd_t* pd_entry) {
    pt_t* pt = (pt_t*)getframe(PT_FRAME,currpid,0);
    if(pt == NULL) {
        kprintf("Insufficient Memory!\n");
        kprintf("Killing the process.\n");
        kill(currpid);
        return SYSERR;
    }
    int j, n_pt_entries = NBPG / (sizeof(pt_t));
    /* Make all entries in the new page table writable */
    for(j=0; j<n_pt_entries; j++) {
        pt[j].pt_write = 1;
    }
    /* Add new page table's address to pd_entry */
    pd_entry->pd_pres = 1;
    pd_entry->pd_base = (uint32)pt >> 12;
    return OK;
}
status mapvaddr(uint32 vaddr) {
    int i = (vaddr & 0xFFC00000)>>22;
    int j = (vaddr & 0x003FF000)>>12;
    //kprintf("i=0x%08X, j=0x%08X\n",i,j);
    pd_t* pd = (pd_t*)read_cr3();
    pd_t* pd_entry = &pd[i];
    if(pd_entry->pd_pres == 0 
        && fill_pd_entry(pd_entry) == SYSERR) {
        return SYSERR;
    }
    pt_t* pt = (pt_t*)(pd_entry->pd_base << 12);
    pt_t* pt_entry = &pt[j];
    if(pt_entry->pt_pres == 0
        && fill_pt_entry(pt_entry) == SYSERR) {
        return SYSERR;
    }
    /* Invalidate TLB for this vaddr */
    asm("pushl %eax");
    asm("movl pfla, %eax");        
    asm("invlpg pfla");
    asm("popl %eax");
    return OK;
}
bool8 is_valid_vaddr(uint32 vaddr) {
    if(vaddr < VFRAME0 * NBPG || vaddr >= 0x90000000) {
        return FALSE;
    }
    struct vmemblk* vl = (proctab[currpid].prvmemlist)->mnext;
    while(vl != NULL) {
        if((uint32) vl->mbegin > vaddr) {
            return TRUE;
        } 
        vl = vl->mnext;
    }
    return FALSE;
}
void pfhandler() {
    pfla = (uint32) read_cr2();
    //kprintf("Page fault handler is called with errcode: %d\n",pferrcode);
    //kprintf("PFLA is 0x%08X\n",pfla);
    hook_pfault(pfla);
    if(!is_valid_vaddr(pfla)) {
        kprintf("Invalid memory access on address 0x%08X\n",pfla);
        kprintf("Killing the process\n");
        kill(currpid);
        return;
    }
    mapvaddr(pfla);
    return;
}
