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

void
sunCallSetup(SunCall *c, SunProg *prog, uint proc)
{
	c->rpc.prog = prog->prog;
	c->rpc.vers = prog->vers;
	c->rpc.proc = proc>>1;
	c->rpc.iscall = !(proc&1);
	c->type = proc;
}
