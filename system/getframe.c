/* getframe.c - getframe */

#include <xinu.h>

/*------------------------------------------------------------------------
 *  getframe -  Allocate a frame, returning lowest word address
 *------------------------------------------------------------------------
 */
char* next_free = (char *) (FRAME0 * NBPG);
char *getframe() {
	intmask mask = disable();
    char* frame = next_free;
    next_free += NBPG;
    memset(frame,0x00,NBPG);
    kprintf("getframe returning 0x%08X\n",frame);
	restore(mask);
    return frame;
}
