/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"

#include "ureg.h"

static void
scrprint(char *fmt, ...)
{
	char buf[128];
	va_list va;
	int n;

	va_start(va, fmt);
	n = vseprint(buf, buf+sizeof buf, fmt, va)-buf;
	va_end(va);
	putstrn(buf, n);
}

static char*
getline(void)
{
	static char buf[128];
	int i, c;

	for(;;){
		for(i=0; i<nelem(buf) && (c=uartgetc()) != '\n'; i++){
			DBG("%c...", c);
			buf[i] = c;
		}

		if(i < nelem(buf)){
			buf[i] = 0;
			return buf;
		}
	}
}

static void*
addr(char *s, Ureg *ureg, char **p)
{
	uint64_t a;

	a = strtoul(s, p, 16);
	if(a < sizeof(Ureg))
		return ((uint8_t*)ureg)+a;
	return (void*)a;
}

static void
talkrdb(Ureg *ureg)
{
	uint8_t *a;
	char *p, *req;

	delconsdevs();		/* turn off serial console and kprint */
//	scrprint("Plan 9 debugger\n");
	iprint("Edebugger reset\n");
	for(;;){
		req = getline();
		switch(*req){
		case 'r':
			a = addr(req+1, ureg, nil);
			DBG("read %#p\n", a);
			iprint("R%.8lux %.2ux %.2ux %.2ux %.2ux\n",
				strtoul(req+1, 0, 16), a[0], a[1], a[2], a[3]);
			break;

		case 'w':
			a = addr(req+1, ureg, &p);
			*(uint32_t*)a = strtoul(p, nil, 16);
			iprint("W\n");
			break;
/*
 *		case Tmput:
			n = min[4];
			if(n > 4){
				mesg(Rerr, Ecount);
				break;
			}
			a = addr(min+0);
			scrprint("mput %.8lux\n", a);
			memmove(a, min+5, n);
			mesg(Rmput, mout);
			break;
 *
 */
		default:
			DBG("unknown %c\n", *req);
			iprint("Eunknown message\n");
			break;
		}
	}
}

void
rdb(void)
{
	splhi();
	iprint("rdb...");
	callwithureg(talkrdb);
}
