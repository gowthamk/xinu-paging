#include<xinu.h>
/*------------------------------------------------------------------------
 *
 * initialize_paging - Initialize virtual memory and demand paging.
 *
 *------------------------------------------------------------------------
 */
pd_t* nullpdir = NULL;
pd_t* get_nullpdir() {
    return nullpdir;
}
pd_t* initialize_paging2(pid32 pid) {
    pd_t* pdir = (pd_t*)getframe(PD_FRAME,pid,0);
    int i;
    int n_pd_entries = NBPG / (sizeof(pd_t));
    /* All page tables are writable */
    for(i=0;i<n_pd_entries; i++) {
        pdir[i].pd_write = 1;
    }
    /* First four page tables are global page tables */
    i=0;
    while(TRUE) {
        /* ith page table is present in the memory */
        pdir[i].pd_pres = 1;
        pdir[i].pd_base = nullpdir[i].pd_base;
        kprintf("%dth entry in page directory is 0x%05X\n",i,pdir[i].pd_base);
        if (i<3) {
            i = i+1;
        } else if (i==3) {
            i = 576;
        } else if (i==576) {
            break;
        }
    }
    return pdir;
}
pd_t* initialize_paging(pid32 pid) {
    if (nullpdir != NULL) {
        return initialize_paging2(pid);
    }
    pd_t* pdir = (pd_t*)getframe(PD_FRAME,pid,0);
    pt_t* ptab;
    int i,j;
    int n_pd_entries = NBPG / (sizeof(pd_t));
    int n_pt_entries = NBPG / (sizeof(pt_t));
    /* All page tables are writable */
    for(i=0;i<n_pd_entries; i++) {
        pdir[i].pd_write = 1;
    }
    /* First four page tables are global page tables */
    i=0;
    while(TRUE) {
        ptab = (pt_t*)getframe(GLOBAL_PT_FRAME,pid,0);
        for(j=0; j<n_pt_entries; j++) {
            /* jth entry in ith page table is (i*2^10)+j*/
            ptab[j].pt_base = (i<<10)+j;
            /* The page is writable and present */
            ptab[j].pt_write = 1;
            ptab[j].pt_pres = 1;
        }
        /* ith page table is present in the memory */
        pdir[i].pd_pres = 1;
        pdir[i].pd_base = (uint32)ptab >> 12;
        kprintf("%dth entry in page directory is 0x%05X\n",i,pdir[i].pd_base);
        if (i<3) {
            i = i+1;
        } else if (i==3) {
            i = 576;
        } else if (i==576) {
            break;
        }
    }
    nullpdir = pdir;
    return pdir;
}

void enable_paging(pd_t* pdir) {
    write_cr3((unsigned long)pdir);
    kprintf("cr0: %x, cr3 %x\n", read_cr0(), read_cr3());
    unsigned long cr0;
    kprintf("enabling paging\n");
    cr0  = read_cr0 ();
    cr0 |= 0x80000001;
    write_cr0 (cr0);
    cr0 = read_cr0 ();
    kprintf("cr0: %x, cr3 %x\n", read_cr0(), read_cr3());
}


