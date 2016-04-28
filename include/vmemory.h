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

/* Define frame type */
typedef byte frame_t;

/* Define various frame types */
#define PT_FRAME 1
#define PD_FRAME 2
#define GLOBAL_PT_FRAME 3
#define DEFAULT_FRAME 4

/* in file getframe.c */
extern	char *getframe(frame_t);
extern status frame_ref_inc(uint32);
extern void print_ipt_stats();

