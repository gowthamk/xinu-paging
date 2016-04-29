/* getframe.c - getframe */

#include <xinu.h>

ipt_t ipt[NFRAMES];

void print_ipt_stats() {
    int pts = 0, used = 0;
    int i;
    for (i=0; i<NFRAMES; i++) {
        if (ipt[i].is_used == 1) {
            used ++;
        }
        if (ipt[i].is_pt == 1) {
            pts++;
        }
        if (ipt[i].is_pt == 1 && ipt[i].is_used == 0) {
            kprintf("!! Frame %d is paging ds, but not being used!\n",i);
        }
        if (ipt[i].is_pt == 1 && ipt[i].ref == 0) {
            kprintf("!! Frame %d is paging ds, but its ref is zero!\n",i);
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
 * Decrement the ref count of the frame that contains the given physical
 * address. If count is 0 and the frame is a pt frame, is_used is unset.
 */
status frame_ref_dec(uint32 addr) {
    if (addr < FRAME0*NBPG || addr >= (FRAME0 + NFRAMES) * NBPG) {
        kprintf("frame_ref_inc: physical address is invalid!\n");
        return SYSERR;
    }
    int i = (addr - (FRAME0*NBPG)) / NBPG;
    ipt[i].ref--;
    if(ipt[i].ref <= 0 && ipt[i].is_pt == 1) {
        ipt[i].is_used = 0;
    }
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
 * Get the page table entry corresponing to the given vfno for the current
 * process.
 */
pt_t* pt_lookup(int vfno) {
    uint32 vaddr = vaddr_of(vfno);
    int i = (vaddr & 0xFFC00000)>>22;
    int j = (vaddr & 0x003FF000)>>12;
    pd_t* pd = (pd_t*)read_cr3();
    pd_t* pd_entry = &pd[i];
    if(pd_entry->pd_pres == 0) {
        kprintf("The PD entry for evicting frame is not present!\n");
        return NULL;
    }
    pt_t* pt = (pt_t*)(pd_entry->pd_base << 12);
    pt_t* pt_entry = &pt[j];
    if(pt_entry->pt_pres == 0) {
        kprintf("!!Warning: PT entry for evicting frame is not present!\n");
    }
    return pt_entry;
}
/*
 * Evicts the frame #pfno. 
 */
status evict_frame(uint32 pfno) {
    if(pfno < 0 || pfno >= NFRAMES) {
        kprintf("evict_frame: invalid pfno.\n");
        return SYSERR;
    }
    kprintf("Physical frame # %d being evicted\n",pfno);
    uint32 vfno = ipt[pfno].vfno;
    /* Write to the backing store */
    pt_t* pt_entry = pt_lookup(vfno);
    char* src = (char*)(pt_entry->pt_base << 12);
    if(src==NULL){
        return SYSERR;
    }
    bsd_t bs = proctab[currpid].prbs;
    if(bs == -1) {
        kprintf("Invalid backing store for pid %d.\n",currpid);
        return SYSERR;
    }
    int retries = 4, status;
    kprintf("Writing page at 0x%08X to bs... ",src);
    while((status = write_bs(src,bs,vfno)) != OK  && retries > 0) {
        retries --;
    }
    if(status != OK) {
        kprintf("FAILED!\n");
        return SYSERR;
    } else {
        kprintf("OK\n");
    }

    /* Set page table entry referring to pfno as not present*/
    pt_entry->pt_pres = 0;

    /* Decrement ref count of pt frame.*/
    frame_ref_dec((uint32) pt_entry);

    /* Set ipt entry of pfno as not used */
    ipt[pfno].is_used = 0;
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
 *------------------------------------------------------------------------
 */
char *getframe(frame_t ft, uint32 vfno) {
	intmask mask = disable();
    int i;
    for(i=0; i<NFRAMES; i++) {
        if(!ipt[i].is_used) {
            break;
        }
    }
    if(i >= NFRAMES) {
        i = find_evictable();
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
        kprintf("IPT map: (pid(%d),vfno(%d)) --> pfno(%d)\n",currpid,vfno,i);
        break;

        default:
        kprintf("getframe: invalid frame type\n");
        return (char*)SYSERR;
    }
    ipt[i].is_used = 1;
    ipt[i].pid = currpid;
    ipt[i].timestamp = get_logiclk_time();
    char* frame = (char *) ((FRAME0 + i)*NBPG);
    memset(frame,0x00,NBPG);
    //kprintf("getframe returning 0x%08X\n",frame);
    //print_ipt_stats();
	restore(mask);
    return frame;
}


