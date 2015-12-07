/* Copyright (c) 2011 The Regents of the University of California
 * Barret Rhoden <brho@cs.berkeley.edu>
 * See LICENSE for details.
 *
 * x86-specific Kernel debugging headers and static inlines */

#ifndef ROS_KERN_ARCH_KDEBUG_H
#define ROS_KERN_ARCH_KDEBUG_H

#include <ros/common.h>
#include <arch/x86.h>

#include <stdio.h>

// Debug information about a particular instruction pointer
typedef struct eipdebuginfo {
	const char *eip_file;		// Source code filename for EIP
	int eip_line;				// Source code linenumber for EIP

	const char *eip_fn_name;	// Name of function containing EIP
								//  - Note: not null terminated!
	int eip_fn_namelen;			// Length of function name
	uintptr_t eip_fn_addr;		// Address of start of function
	int eip_fn_narg;			// Number of function arguments
} eipdebuginfo_t;

int debuginfo_eip(uintptr_t eip, eipdebuginfo_t *info);
void *debug_get_fn_addr(char *fn_name);

/* Returns a PC/EIP in the function that called us, preferably near the call
 * site.  Returns 0 when we can't jump back any farther. */
static inline uintptr_t get_caller_pc(void)
{
	unsigned long *ebp = (unsigned long*)read_bp();
	if (!ebp)
		return 0;
	/* this is part of the way back into the call() instruction's bytes
	 * eagle-eyed readers should be able to explain why this is good enough, and
	 * retaddr (just *(ebp + 1) is not) */
	return *(ebp + 1) - 1;
}

#endif /* ROS_KERN_ARCH_KDEBUG_H */
