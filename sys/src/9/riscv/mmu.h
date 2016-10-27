/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/* PML4E/PDPE/PDE/PTE */
#define PteP            0x0000000000000001	/* Valid */
#define PteR		0x0000000000000002	/* Read */
#define PteW		0x0000000000000004	/* Write */
#define PteRW		0x0000000000000006	/* Read/Write */
#define PteX		0x0000000000000008	/* Read */
#define PteFinal        0x000000000000000e      /* Last PTE in the chain */
#define PteU		0x0000000000000010	/* User/Supervisor */
#define PteA		0x0000000000000040	/* Accessed */
#define PteD		0x0000000000000080	/* Dirty */
#define PteG		0x0000000000000020	/* Global */
