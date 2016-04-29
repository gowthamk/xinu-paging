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

/*------------------------------------------------------------------------
 *  getframe -  Allocate a frame, returning lowest word address
 *------------------------------------------------------------------------
 */
char *getframe(frame_t ft) {
	intmask mask = disable();
    int i;
    for(i=0; i<NFRAMES; i++) {
        if(!ipt[i].is_used) {
            break;
        }
    }
    if(i == -1) {
        kprintf("Eviction Unimpl.\n");
    }
    switch (ft) {
        case PT_FRAME:
        ipt[i].is_pt = 1;
        ipt[i].ref = 0;
        break;

        case PD_FRAME:
        ipt[i].is_pt = 1;
        ipt[i].ref = 100; /* some non-zero value */
        break;

        case GLOBAL_PT_FRAME:
        ipt[i].is_pt = 1;
        ipt[i].ref = 1024;
        break;

        case DEFAULT_FRAME:
        ipt[i].is_pt = 0;
        break;

        default:
        kprintf("getframe: invalid frame type\n");
        return (char*)SYSERR;
    }
    ipt[i].is_used = 1;
    ipt[i].pid = currpid;
    char* frame = (char *) ((FRAME0 + i)*NBPG);
    memset(frame,0x00,NBPG);
    //kprintf("getframe returning 0x%08X\n",frame);
    //print_ipt_stats();
	restore(mask);
    return frame;
}

/*
 * Increment the ref count of the frame that contains the given physical
 * address.
 */
status frame_ref_inc(uint32 addr) {
    if (addr<FRAME0 || addr >= FRAME0 + (NFRAMES * NBPG)) {
        kprintf("frame_ref_inc: physical address is invalid!\n");
        return SYSERR;
    }
    int i = (addr - (FRAME0*NBPG)) / NBPG;
    ipt[i].ref++;
    return OK;
}

