/* virtual memory related things */
struct	vmemblk	{			/* See roundmb & truncmb	*/
	struct	vmemblk	*mnext;		/* Ptr to next free memory blk	*/
	uint32	mlength;		/* Size of blk (includes memblk)*/
    char*   mbegin;         /* The beginning virtual address of this blk */
	};
/* Beginning frame of virtual address */
#define VFRAME0 (FRAME0 + NFRAMES)
//#define VFRAME0 4096
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

/* Virtual frame number of given virtual address */
#define vframe_of(vaddr) (((uint32) vaddr >> 12) - (FRAME0 + NFRAMES))
#define vaddr_of(vfno) ((vfno + FRAME0 + NFRAMES)<<12)

/* in file getframe.c */
extern	char *getframe(frame_t,uint32);
extern status frame_ref_inc(uint32);
extern void print_ipt_stats();
extern status evict_frame(uint32);

