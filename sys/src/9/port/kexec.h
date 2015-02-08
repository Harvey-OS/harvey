/* 
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

typedef struct Kvalue Kvalue;
typedef struct Kexecgrp Kexecgrp;


/* Kexec structures */
struct Kvalue
{
	uintptr addr;
	uvlong size;
	int	len;
	int inuse;
	Kvalue	*link;
	Qid	qid;
};

struct Kexecgrp
{
	Ref;
	RWlock;
	Kvalue	**ent;
	int	nent;
	int	ment;
	ulong	path;	/* qid.path of next Kvalue to be allocated */
	ulong	vers;	/* of Kexecgrp */
};

void	kforkexecac(Proc*, int, char*, char**);
Proc*	setupseg(int core);
