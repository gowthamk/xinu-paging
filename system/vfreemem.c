/* vfreemem.c - vfreemem */

#include <xinu.h>

/*------------------------------------------------------------------------
 *  vfreemem  -  Free a memory blkaddr, returning the blkaddr to the free list
 *------------------------------------------------------------------------
 */
syscall	vfreemem(
	  char		*blkaddr,	/* Pointer to memory blkaddr	*/
	  uint32	nbytes		/* Size of blkaddr in bytes	*/
	)
{
	intmask	mask;			/* Saved interrupt mask		*/
	struct	vmemblk	*next, *prev;
	uint32	top;

	mask = disable();
	if ((nbytes == 0) || ((uint32) blkaddr < (uint32) vminheap)
			  || ((uint32) blkaddr > (uint32) vmaxheap)) {
		restore(mask);
        kprintf("vfreemem returning SYSERR\n");
		return SYSERR;
	}

    struct vmemblk* vmemlist = proctab[currpid].prvmemlist;
	prev = vmemlist;			/* Walk along free list	*/
	next = vmemlist->mnext;
	while ((next != NULL) && (next->mbegin < blkaddr)) {
		prev = next;
		next = next->mnext;
	}

	if (prev == vmemlist) {		/* Compute top of previous blkaddr*/
		top = (uint32) NULL;
	} else {
		top = (uint32) (prev->mbegin) + prev->mlength;
	}

	/* Ensure new blkaddr does not overlap previous or next blkaddrs	*/

	if (((prev != vmemlist) && (uint32) blkaddr < top)
	    || ((next != NULL)	&& (uint32) blkaddr+nbytes>(uint32)next->mbegin)) {
		restore(mask);
        kprintf("vfreemem returning SYSERR\n");
		return SYSERR;
	}

	vmemlist->mlength += nbytes;

	/* Either coalesce with previous blkaddr or add to free list */
    
    bool8 lc = (top == (uint32) blkaddr), 
          rc = (next!=NULL && blkaddr + nbytes == next->mbegin);

	if (lc && rc) { 
		prev->mlength += nbytes + next->mlength;
        prev->mnext = next->mnext;
        freemem((char*)next,sizeof(struct vmemblk));
	} else if (lc) {
        prev->mlength += nbytes;
    } else if (rc) {
        next->mbegin = blkaddr;
        next->mlength += nbytes;
    } else {
        struct vmemblk* newblk = (struct vmemblk*)getmem(sizeof(struct vmemblk));
        newblk->mbegin = blkaddr;
        newblk->mlength = nbytes;
        prev->mnext = newblk;
        newblk->mnext = next;
	}

	restore(mask);
	return OK;
}
