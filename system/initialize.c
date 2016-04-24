/* initialize.c - nulluser, sysinit, sizmem */

/* Handle system initialization and become the null process */

#include <xinu.h>
#include <string.h>

extern	void	start(void);	/* Start of Xinu code			*/
extern	void	*_end;		/* End of Xinu code			*/

/* Function prototypes */

extern	void main(void);	/* Main is the first process created	*/
extern	void xdone(void);	/* System "shutdown" procedure		*/
static	void sysinit(); 	/* Internal system initialization	*/
static  void initialize_paging(); /* Paging initialization */
extern	void meminit(void);	/* Initializes the free memory list	*/

/* Declarations of major kernel variables */

struct	procent	proctab[NPROC];	/* Process table			*/
struct	sentry	semtab[NSEM];	/* Semaphore table			*/
struct	memblk	memlist;	/* List of free memory blocks		*/

/* Active system status */

int	prcount;		/* Total number of live processes	*/
pid32	currpid;		/* ID of currently executing process	*/

bool8   PAGE_SERVER_STATUS;    /* Indicate the status of the page server */
sid32   bs_init_sem;
/*------------------------------------------------------------------------
 * nulluser - initialize the system and become the null process
 *
 * Note: execution begins here after the C run-time environment has been
 * established.  Interrupts are initially DISABLED, and must eventually
 * be enabled explicitly.  The code turns itself into the null process
 * after initialization.  Because it must always remain ready to execute,
 * the null process cannot execute code that might cause it to be
 * suspended, wait for a semaphore, put to sleep, or exit.  In
 * particular, the code must not perform I/O except for polled versions
 * such as kprintf.
 *------------------------------------------------------------------------
 */
/* For paging */
unsigned long tmp;

void	nulluser()
{	
	struct	memblk	*memptr;	/* Ptr to memory block		*/
	uint32	free_mem;		/* Total amount of free memory	*/
	
	/* Initialize the system */
		
	sysinit();

	kprintf("\n\r%s\n\n\r", VERSION);
	
	/* Output Xinu memory layout */
	free_mem = 0;
	for (memptr = memlist.mnext; memptr != NULL;
						memptr = memptr->mnext) {
		free_mem += memptr->mlength;
	}
	kprintf("%10d bytes of free memory.  Free list:\n", free_mem);
	for (memptr=memlist.mnext; memptr!=NULL;memptr = memptr->mnext) {
	    kprintf("           [0x%08X to 0x%08X]\r\n",
		(uint32)memptr, ((uint32)memptr) + memptr->mlength - 1);
	}

	kprintf("%10d bytes of Xinu code.\n",
		(uint32)&etext - (uint32)&text);
	kprintf("           [0x%08X to 0x%08X]\n",
		(uint32)&text, (uint32)&etext - 1);
	kprintf("%10d bytes of data.\n",
		(uint32)&ebss - (uint32)&data);
	kprintf("           [0x%08X to 0x%08X]\n\n",
		(uint32)&data, (uint32)&ebss - 1);

	/* Create the RDS process */

	rdstab[0].rd_comproc = create(rdsprocess, RD_STACK, RD_PRIO,
					"rdsproc", 1, &rdstab[0]);
	if(rdstab[0].rd_comproc == SYSERR) {
		panic("Cannot create remote disk process");
	}
	resume(rdstab[0].rd_comproc);

	/* Enable interrupts */

	enable();

	/* Create a process to execute function main() */

	resume (
	   create((void *)main, INITSTK, INITPRIO, "Main process", 0,
           NULL));

	/* Become the Null process (i.e., guarantee that the CPU has	*/
	/*  something to run when no other process is ready to execute)	*/

	while (TRUE) {
		;		/* Do nothing */
	}

}

/*------------------------------------------------------------------------
 *
 * initalize_paging - Initialize virtual memory and demand paging.
 *
 *------------------------------------------------------------------------
 */
void write_cr0(unsigned long n) {

  intmask ps = disable();

  tmp = n;
  asm("pushl %eax");
  asm("movl tmp, %eax");        /* mov (move) value at tmp into %eax register. 
                       "l" signifies long (see docs on gas assembler)   */
  asm("movl %eax, %cr0");
  asm("popl %eax");
  restore(ps);
}
unsigned long read_cr0(void) {
  unsigned long local_tmp;
  intmask ps;
  ps = disable();
  asm("pushl %eax");
  asm("movl %cr0, %eax");
  asm("movl %eax, tmp");
  asm("popl %eax");
  local_tmp = tmp;
  restore(ps);
  return local_tmp;
}

void write_cr3(unsigned long n) {
  intmask ps;
  ps = disable();
  tmp = n;
  asm("pushl %eax");
  asm("movl tmp, %eax");                
  asm("movl %eax, %cr3");
  asm("popl %eax");
  restore(ps);
}

unsigned long read_cr3(void) {
  unsigned long local_tmp;
  intmask ps;
  ps = disable();
  asm("pushl %eax");
  asm("movl %cr3, %eax");
  asm("movl %eax, tmp");
  asm("popl %eax");
  local_tmp = tmp;
  restore(ps);
  return local_tmp;
}

void enable_paging()
{
    unsigned long cr0;
    kprintf("enabling paging\n");
    cr0  = read_cr0 ();
    cr0 |= 0x80000001;
    write_cr0 (cr0);
    cr0 = read_cr0 ();
    kprintf("cr0: %x, cr3 %x\n", read_cr0(), read_cr3());
}

static void initialize_paging() {
    pd_t* pdir = (pd_t*)getframe();
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
        ptab = (pt_t*)getframe();
        for(j=0; j<n_pt_entries; j++) {
            /* jth entry in ith page table is (i*2^10)+j*/
            ptab[j].pt_base = (i<<10)+j;
            /* It is writable and the page is present */
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
    write_cr3((unsigned long)pdir);
    kprintf("cr0: %x, cr3 %x\n", read_cr0(), read_cr3());
    return;
}

/*------------------------------------------------------------------------
 *
 * sysinit  -  Initialize all Xinu data structures and devices
 *
 *------------------------------------------------------------------------
 */
static	void	sysinit()
{
	int32	i;
	struct	procent	*prptr;		/* Ptr to process table entry	*/
	struct	sentry	*semptr;	/* Ptr to semaphore table entry	*/

	/* Platform Specific Initialization */

	platinit();

	/* Initialize the interrupt vectors */

	initevec();
	
	/* Initialize free memory list */
	
	meminit();

	/* Initialize system variables */

	/* Count the Null process as the first process in the system */

	prcount = 1;

	/* Scheduling is not currently blocked */

	Defer.ndefers = 0;

	/* Initialize process table entries free */

	for (i = 0; i < NPROC; i++) {
		prptr = &proctab[i];
		prptr->prstate = PR_FREE;
		prptr->prname[0] = NULLCH;
		prptr->prstkbase = NULL;
		prptr->prprio = 0;
	}

	/* Initialize the Null process entry */	

	prptr = &proctab[NULLPROC];
	prptr->prstate = PR_CURR;
	prptr->prprio = 0;
	strncpy(prptr->prname, "prnull", 7);
	prptr->prstkbase = getstk(NULLSTK);
	prptr->prstklen = NULLSTK;
	prptr->prstkptr = 0;
	currpid = NULLPROC;
	
	/* Initialize semaphores */

	for (i = 0; i < NSEM; i++) {
		semptr = &semtab[i];
		semptr->sstate = S_FREE;
		semptr->scount = 0;
		semptr->squeue = newqueue();
	}

	/* Initialize buffer pools */

	bufinit();

	/* Create a ready list for processes */

	readylist = newqueue();

	/* Initialize the real time clock */

	clkinit();

	for (i = 0; i < NDEVS; i++) {
		init(i);
	}

        PAGE_SERVER_STATUS = PAGE_SERVER_INACTIVE;
        bs_init_sem = semcreate(1);

    /* initialize paging */
    initialize_paging();
    enable_paging();

	return;
}

int32	stop(char *s)
{
	kprintf("%s\n", s);
	kprintf("looping... press reset\n");
	while(1)
		/* Empty */;
}

int32	delay(int n)
{
	DELAY(n);
	return OK;
}
