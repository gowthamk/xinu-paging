/* getframe.c - getframe */

#include <xinu.h>

ipt_t ipt[NFRAMES];

void print_ipt() {
    int i;
    for(i=0;i<NFRAMES;i++) {
        kprintf("[U(%d),PT(%d),pid(%d)]\t",ipt[i].is_used
                ,ipt[i].is_pt, ipt[i].pid);
    }
    return;
}
void print_ipt_stats() {
    int pts = 0, used = 0;
    int i;
    for (i=0; i<NFRAMES; i++) {
        if(ipt[i].is_used == 0) {
            continue;
        }
        used ++;
        if (ipt[i].is_pt == 1) {
            pts++;
        }
        if (ipt[i].is_pt == 1 && ipt[i].ref == 0) {
            kprintf("!! Frame %d is paging ds under use, but its ref is zero!\n",i);
        }
    }
    kprintf("----------------------------------------------------\n");
    kprintf("Total frames: %d. Used: %d. Paging DS Frames: %d\n"
                ,NFRAMES, used, pts);
    kprintf("----------------------------------------------------\n");
    return;
}

/*
 * Logical clock time. Ticks everytime it is observed.
 */
uint32 logiclk = 0;
uint32 get_logiclk_time() {
    return logiclk++;
}
/*
 * Finds a page to evict using FIFO replacement policy.
 */
int find_evictable_fifo() {
    int i, fst=-1, min = get_logiclk_time();
    for(i=0; i<NFRAMES; i++) {
        if(ipt[i].is_pt && ipt[i].ref > 0) {
            continue;
        }
        if(ipt[i].is_used == 0) {
            kprintf("Unused frame found during eviction!\n");
            return i;
        }
        if(ipt[i].timestamp < min) {
            min = ipt[i].timestamp;
            fst = i;
        }
    }
    return fst;
}
/*
 * Finds a page to evict using the current replacement policy.
 */
int find_evictable() {
    if(PG_REPLACEMENT_POLICY == FIFO) {
        return find_evictable_fifo();
    } else {
        kprintf("Only FIFO replacement is implemented\n");
        return -1;
    }
    return -1;
}
/*
 * Increment the ref count of the frame that contains the given physical
 * address.
 */
status frame_ref_inc(uint32 addr) {
    if (addr < FRAME0*NBPG || addr >= (FRAME0 + NFRAMES) * NBPG) {
        kprintf("frame_ref_inc: physical address is invalid!\n");
        return SYSERR;
    }
    int i = (addr - (FRAME0*NBPG)) / NBPG;
    ipt[i].ref++;
    return OK;
}
/*
 * Given a pid and a virtual frame number, returns the corresponding physical
 * frame number, if one exists. Returns -1 otherwise.
 */
int ipt_lookup(pid32 pid,int vfno) {
    int i;
    for(i=0; i<NFRAMES; i++) {
        if(ipt[i].pid == pid && ipt[i].vfno == vfno) {
            return i;
        }
    }
    return -1;
}

/*
 * Evicts the frame #pfno. 
 */
status evict_frame(uint32 pfno) {
    if(pfno < 0 || pfno >= NFRAMES) {
        kprintf("evict_frame: invalid pfno(%d).\n",pfno);
        return SYSERR;
    }
    //kprintf("Physical frame # %d being evicted\n",pfno);
    uint32 vfno = ipt[pfno].vfno;
    /*
     * Get the page table entry corresponing to the given vfno for the current
     * process.
     */
    uint32 vaddr = vaddr_of(vfno);
    int i = (vaddr & 0xFFC00000)>>22;
    int j = (vaddr & 0x003FF000)>>12;
    pd_t* pd = (pd_t*) proctab[ipt[pfno].pid].prpdir;
    pd_t* pd_entry = &pd[i];
    if(pd_entry->pd_pres == 0) {
        kprintf("The PD entry for evicting frame is not present!\n");
        return NULL;
    }
    pt_t* pt = (pt_t*)(pd_entry->pd_base << 12);
    pt_t* pt_entry = &pt[j];
    if(pt_entry->pt_pres == 0) {
        kprintf("!!Warning: PT entry (0x%08X) for evicting frame is not present!\n", pt_entry);
    }
    
    /* Write to the backing store */
    char* src = (char*)(pt_entry->pt_base << 12);
    if(src==NULL){
        kprintf("pt_entry->pt_base is null!\n");
        return SYSERR;
    }
    bsd_t bs = proctab[ipt[pfno].pid].prbs;
    if(bs == -1) {
        kprintf("Invalid backing store for pid %d.\n",ipt[pfno].pid);
        //print_ipt();
        return SYSERR;
    }
    int retries = 4, status;
    hook_pswap_out(pfno+FRAME0,vfno+VFRAME0);
    //kprintf("Writing page at 0x%08X to bs(%d)... ",src,bs);
    while((status = write_bs(src,bs,vfno)) != OK  && retries > 0) {
        retries --;
    }
    if(status != OK) {
        kprintf("FAILED!\n");
        return SYSERR;
    } else {
        kprintf("OK\n");
    }

    /* Set ipt entry of pfno as not used */
    ipt[pfno].is_used = 0;

    /* Set page table entry referring to pfno as not present*/
    pt_entry->pt_pres = 0;

    /* Decrement ref count of pt frame. If ref count reaches zero, 
     * mark the frame as free, and set page dir entry referring to this page
     * table as not present. */
    uint32 addr = (uint32) pt_entry;
    int ptpfno = (addr - (FRAME0*NBPG)) / NBPG;
    ipt[ptpfno].ref--;
    if(ipt[ptpfno].ref <= 0 && ipt[ptpfno].is_pt == 1) {
        //kprintf("Freeing a PT frame\n");
        hook_ptable_delete(ptpfno+FRAME0);
        pd_entry->pd_pres = 0;
        ipt[ptpfno].is_used = 0;
        ipt[ptpfno].is_pt = 0;
    }

    return OK;
}
/*
 * Evicts the vframe #vfno of the current process. Calls evict_frame.
 */
status evict_vframe(uint32 vfno) {
    int pfno = ipt_lookup(currpid,vfno);
    kprintf("Frame number %d of current process being evicted\n",vfno);
    return evict_frame((uint32) pfno);
}
/*------------------------------------------------------------------------
 *  getframe -  Allocate a frame of type ft, map (currpid,vfno) to physical
 *  frame number in ipt if ft=DEFAULT_FRAME, and returning the physical frame
 *  adrress.
 *  Note: getframe may evict a frame, if necessary. This in itself is not a
 *  problem; page fault handler can get the frame back into memory. However, if
 *  the evicting frame is the last the frame pointed by its page table, then
 *  getframe deallocates (not evicts) the page table: it marks it as not used,
 *  and sets its pd entry as not present. This is safe as long as the page table
 *  has no external references. If there are external references to the page
 *  table, then the ref count of the page table has to be incremented to ensure
 *  that the page table is not deallocated.
 *------------------------------------------------------------------------
 */
char *getframe(frame_t ft, pid32 pid, uint32 vfno) {
	intmask mask = disable();
    int i;
    for(i=0; i<NFRAMES; i++) {
        if(!ipt[i].is_used) {
            break;
        }
    }
    if(i >= NFRAMES) {
        i = find_evictable();
        if(i==-1) {
            //print_ipt();
            kprintf("\nCould not find an evictable frame!\n");
            return NULL;
        }
        if(evict_frame(i) != OK) {
            kprintf("getframe: No free frames. Could not evict frames\n");
            return NULL;
        }
    }
    uint32 invalid_vfno = 0xFFFFFFFF;
    switch (ft) {
        case PT_FRAME:
        ipt[i].is_pt = 1;
        ipt[i].ref = 0;
        ipt[i].vfno = invalid_vfno;
        hook_ptable_create(i+FRAME0);
        break;

        case PD_FRAME:
        ipt[i].is_pt = 1;
        ipt[i].ref = 1024; /* Some non-zero value. Never decremented */
        ipt[i].vfno = invalid_vfno;
        break;

        case GLOBAL_PT_FRAME:
        ipt[i].is_pt = 1;
        ipt[i].ref = 1024; /* Some non-zero value. Never decremented */
        ipt[i].vfno = invalid_vfno;
        break;

        case DEFAULT_FRAME:
        ipt[i].is_pt = 0;
        ipt[i].vfno = vfno;
        //kprintf("IPT map: (pid(%d),vfno(%d)) --> pfno(%d)\n",pid,vfno,i);
        break;

        default:
        kprintf("getframe: invalid frame type\n");
        return (char*)SYSERR;
    }
    ipt[i].is_used = 1;
    ipt[i].pid = pid;
    ipt[i].timestamp = get_logiclk_time();
    char* frame = (char *) ((FRAME0 + i)*NBPG);
    memset(frame,0x00,NBPG);
    //kprintf("getframe returning 0x%08X\n",frame);
    //print_ipt_stats();
	restore(mask);
    return frame;
}

status free_proc_frames(pid32 pid) {
    int i, freed=0;
    //kprintf("free_proc_frames: ");
    for(i=0; i<NFRAMES; i++) {
        if(ipt[i].pid == pid) {
            if(ipt[i].is_pt == 1) {
                hook_ptable_delete(FRAME0+i);
            }
            ipt[i].is_used = 0;
            ipt[i].is_pt = 0;
            ipt[i].pid = 0;
            freed++;
        }
    }
    //kprintf("freed %d frames\n",freed);
    return OK;
}
