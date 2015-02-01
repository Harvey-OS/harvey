/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
 * Deterministic regexp program.
 */
typedef struct Dreprog Dreprog;
typedef struct Dreinst Dreinst;
typedef struct Drecase Drecase;

struct Dreinst
{
	int isfinal;
	int isloop;
	Drecase *c;
	int nc;
};

struct Dreprog
{
	Dreinst *start[4];
	int ninst;
	Dreinst inst[1];
};

struct Drecase
{
	uint start;
	Dreinst *next;
};

Dreprog* dregcvt(Reprog*);
int dregexec(Dreprog*, char*, int);
Dreprog* Breaddfa(Biobuf *b);
void Bprintdfa(Biobuf*, Dreprog*);

