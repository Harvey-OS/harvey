/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/* io bufs */
void iobufinit(void);
Block* va2block(void*);
void* io2alloc(uint);
Block* bigalloc(void);
Block* sbigalloc(void);
int isbigblock(Block*);

/* bal.c */
physaddr bal(usize);
void bfree(physaddr, usize);
void balinit(physaddr, usize);
void balfreephys(physaddr, usize);
void baldump(void);
