/* pfisr.s - pfisr (x86) */

/*------------------------------------------------------------------------
 * pfisr - Page fault interrupt service routine
 *------------------------------------------------------------------------
 */
		.text
		.globl	pfisr		# Page fault interrupt dispatcher
pfisr:
        popl pferrcode  # Pop page fault error code
		pushal			# Save registers
		cli			# Disable further interrupts

		call	pfhandler # Call high level handler

		sti			# Restore interrupt status
		popal			# Restore registers
		iret			# Return from interrupt
