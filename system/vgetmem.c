/* vgetmem.c - vgetmem */

#include <xinu.h>

/*------------------------------------------------------------------------
 *  vgetmem  -  Allocate virtual memory, returning lowest word address
 *------------------------------------------------------------------------
 */
char  	*vgetmem(
	  uint32	nbytes		/* Size of memory requested	*/
	)
{
	intmask	mask;			/* Saved interrupt mask		*/
	struct	vmemblk	*prev, *curr, *vmemlist;
    char*  nextfree;

	mask = disable();
	if (nbytes == 0) {
		restore(mask);
		return (char *)SYSERR;
	}

    vmemlist = proctab[currpid].prvmemlist;
	prev = vmemlist;
	curr = vmemlist->mnext;
	while (curr != NULL) {			/* Search free list	*/

		if (curr->mlength == nbytes) {	/* Block is exact match	*/
			prev->mnext = curr->mnext;
			vmemlist->mlength -= nbytes;
			restore(mask);
			return (char *)(curr);

		} else if (curr->mlength > nbytes) { /* Split big block	*/
            nextfree = curr->mbegin + nbytes;
            curr->mbegin = nextfree;
			curr->mlength = curr->mlength - nbytes;
			vmemlist->mlength -= nbytes;
			restore(mask);
			return (char *)(curr);
		} else {			/* Move to next block	*/
			prev = curr;
			curr = curr->mnext;
		}
	}
	restore(mask);
	return (char *)SYSERR;
}
