/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <u.h>
#include <libc.h>
#include <thread.h>
#include <sunrpc.h>

SunStatus
sunCallPack(SunProg *prog, uint8_t *a, uint8_t *ea, uint8_t **pa,
	    SunCall *c)
{
	uint8_t *x;
	int (*pack)(uint8_t*, uint8_t*, uint8_t**, SunCall*);

	if(pa == nil)
		pa = &x;
	if(c->type < 0 || c->type >= prog->nproc || (pack=prog->proc[c->type].pack) == nil)
		return SunProcUnavail;
	if((*pack)(a, ea, pa, c) < 0)
		return SunGarbageArgs;
	return SunSuccess;
}

SunStatus
sunCallUnpack(SunProg *prog, uint8_t *a, uint8_t *ea, uint8_t **pa,
	      SunCall *c)
{
	uint8_t *x;
	int (*unpack)(uint8_t*, uint8_t*, uint8_t**, SunCall*);

	if(pa == nil)
		pa = &x;
	if(c->type < 0 || c->type >= prog->nproc || (unpack=prog->proc[c->type].unpack) == nil)
		return SunProcUnavail;
	if((*unpack)(a, ea, pa, c) < 0){
		fprint(2, "in: %.*H unpack failed\n", (int)(ea-a), a);
		return SunGarbageArgs;
	}
	return SunSuccess;
}

SunStatus
sunCallUnpackAlloc(SunProg *prog, SunCallType type, uint8_t *a, uint8_t *ea,
		   uint8_t **pa, SunCall **pc)
{
	uint8_t *x;
	uint size;
	int (*unpack)(uint8_t*, uint8_t*, uint8_t**, SunCall*);
	SunCall *c;

	if(pa == nil)
		pa = &x;
	if(type < 0 || type >= prog->nproc || (unpack=prog->proc[type].unpack) == nil)
		return SunProcUnavail;
	size = prog->proc[type].sizeoftype;
	if(size == 0)
		return SunProcUnavail;
	c = mallocz(size, 1);
	if(c == nil)
		return SunSystemErr;
	c->type = type;
	if((*unpack)(a, ea, pa, c) < 0){
		fprint(2, "in: %.*H unpack failed\n", (int)(ea-a), a);
		free(c);
		return SunGarbageArgs;
	}
	*pc = c;
	return SunSuccess;
}

uint
sunCallSize(SunProg *prog, SunCall *c)
{
	uint (*size)(SunCall*);

	if(c->type < 0 || c->type >= prog->nproc || (size=prog->proc[c->type].size) == nil)
		return ~0;
	return (*size)(c);
}
