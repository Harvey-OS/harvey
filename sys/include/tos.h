/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

typedef struct Tos Tos;

struct Tos {
	uint64_t	cyclefreq;	/* cycle clock frequency if there is one, 0 otherwise */
	int64_t	kcycles;	/* cycles spent in kernel */
	int64_t	pcycles;	/* cycles spent in process (kernel + user) */
	uint32_t	pid;		/* might as well put the pid here */
	uint32_t	clock;
	/* scratch space for kernel use (e.g., mips fp delay-slot execution) */
	uint32_t	kscr[4];

	/*
	 * Fields below are not available on Plan 9 kernels.
	 */
	int	nixtype;	/* role of the core we are running at */
	int	core;		/* core we are running at */
	/* top of stack is here */
};

extern Tos *_tos;
