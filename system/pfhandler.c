/* pfhandler.c - pfhandler */

#include<xinu.h>
long pferrcode;
extern char* pf_test_ptr;
/*------------------------------------------------------------------------
 * pfhandler - high level page fault handler
 *------------------------------------------------------------------------
 */
void pfhandler() {
    kprintf("Page fault handler is called with errcode: %d\n",pferrcode);
    kprintf("PFLA is 0x%08X\n",read_cr2());
    write_cr2((unsigned long)90000000);
    return;
}
