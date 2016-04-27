/* vmeminit.c - set virtual memory bounds for the given size and initialize a 
 *              free list */

#include <xinu.h>
extern void print_vmemlist(struct vmemblk*);
/* Memory bounds */
char *vminheap = (char*) (VFRAME0 * NBPG);
char *vmaxheap = (char*) 0x8FFFFFFF;
struct vmemblk* vmeminit(uint32 hsize) {
    uint32 mlength = hsize * NBPG;
    struct vmemblk* vmemlist = (struct vmemblk*)getmem(sizeof(struct vmemblk));
    /* Meta head of memlist */
    vmemlist->mnext = (struct vmemblk*)getmem(sizeof(struct vmemblk));
    vmemlist->mlength = mlength;
    vmemlist->mbegin = vminheap;
    /* Real head of memlist */
    vmemlist->mnext->mnext = NULL;
    vmemlist->mnext->mlength = mlength;
    vmemlist->mnext->mbegin = vminheap;
    return vmemlist;
}
