/* pfhandler.c - pfhandler */

#include<xinu.h>
long pferrcode;
extern char* pf_test_ptr;
uint32 last_pfla;
/*------------------------------------------------------------------------
 * pfhandler - high level page fault handler
 *------------------------------------------------------------------------
 */
status fill_pt_entry(pt_t* pt_entry) {
    uint32 newpg = (uint32)getframe();
    if((status) newpg == SYSERR) {
        return SYSERR;
    }
    /* Add new page's address to pt_entry */
    pt_entry->pt_pres = 1;
    pt_entry->pt_base = newpg >> 12;
    return OK;
}
status fill_pd_entry(pd_t* pd_entry) {
    pt_t* pt = (pt_t*)getframe();
    if((status) pt == SYSERR) {
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
    return OK;
}
void pfhandler() {
    uint32 pfla = (uint32) read_cr2();
    kprintf("Page fault handler is called with errcode: %d\n",pferrcode);
    kprintf("PFLA is 0x%08X\n",pfla);
    if(pfla == last_pfla) {
        kprintf("Something's wrong\n");
        while(TRUE);
    }
    mapvaddr(pfla);
    last_pfla = pfla;
    return;
}
