/* virtual memory related things */
struct	vmemblk	{			/* See roundmb & truncmb	*/
	struct	vmemblk	*mnext;		/* Ptr to next free memory blk	*/
	uint32	mlength;		/* Size of blk (includes memblk)*/
    char*   mbegin;         /* The beginning virtual address of this blk */
	};
#define VFRAME0 (FRAME0 + NFRAMES)
extern char *vminheap;
extern char *vmaxheap;
extern struct vmemblk* vmeminit(uint32);
