/*  main.c  - main */

#include <xinu.h>
#include <stdio.h>
extern process test_vmemlist(void);
extern process test_vmem_1(void);
extern process test_vmem_2(void);
extern void test_vmem_3(void);
process	main(void)
{

	/* Start the network */
	/* DO NOT REMOVE OR COMMENT THIS CALL */
	netstart();

	/* Initialize the page server */
	/* DO NOT REMOVE OR COMMENT THIS CALL */
	psinit();

	kprintf("\n...creating a shell\n");
	recvclr();
    kprintf("PROCESSES for vmemory test\n");
    resume(vcreate(test_vmem_1, 8192, INITHEAP, 50, "test1",0));
    //  sleep(2);
    //  resume(vcreate(test_vmem_2, 8192, INITHEAP, 50, "test2",0));
    sleep(2);
    test_vmem_3();

    //resume(vcreate(test_vmem, 8192, INITHEAP, 50, "test_vmemlist2",0));
	
    //resume(vcreate(shell, 8192, INITHEAP, 50, "shell", 1, CONSOLE));

	/* Wait for shell to exit and recreate it */

    /*
	while (TRUE) {
		receive();
		sleepms(200);
		kprintf("\n\nMain process recreating shell\n\n");
		resume(vcreate(shell, 4096, INITHEAP, 20, "shell", 1, CONSOLE));
	}
    */
	return OK;
}
