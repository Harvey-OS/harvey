#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"


static struct
{
	Lock;
	Cycintr	*ci;
}cycintrs;

/*
 * called by clockintrsched()
 */
vlong
cycintrnext(void)
{
	Cycintr *ci;
	vlong when;

	ilock(&cycintrs);
	when = 0;
	ci = cycintrs.ci;
	if(ci != nil)
		when = ci->when;
	iunlock(&cycintrs);
	return when;
}

vlong
checkcycintr(Ureg *u, void*)
{
	Cycintr *ci;
	vlong when;

	ilock(&cycintrs);
	while(ci = cycintrs.ci){
		when = ci->when;
		if(when > fastticks(nil)){
			iunlock(&cycintrs);
			return when;
		}
		cycintrs.ci = ci->next;
		iunlock(&cycintrs);
		(*ci->f)(u, ci);
		ilock(&cycintrs);
	}
	iunlock(&cycintrs);
	return 0;
}

void
cycintradd(Cycintr *nci)
{
	Cycintr *ci, **last;

	ilock(&cycintrs);
	last = &cycintrs.ci;
	while(ci = *last){
		if(ci == nci){
			*last = ci->next;
			break;
		}
		last = &ci->next;
	}

	last = &cycintrs.ci;
	while(ci = *last){
		if(ci->when > nci->when)
			break;
		last = &ci->next;
	}
	nci->next = *last;
	*last = nci;
	iunlock(&cycintrs);
}

void
cycintrdel(Cycintr *dci)
{
	Cycintr *ci, **last;

	ilock(&cycintrs);
	last = &cycintrs.ci;
	while(ci = *last){
		if(ci == dci){
			*last = ci->next;
			break;
		}
		last = &ci->next;
	}
	iunlock(&cycintrs);
}
