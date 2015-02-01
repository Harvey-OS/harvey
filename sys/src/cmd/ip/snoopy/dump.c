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
#include <ip.h>
#include <ctype.h>
#include "dat.h"
#include "protos.h"

static void
p_compile(Filter *)
{
}

static char tohex[16] = {
	'0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
	'a', 'b', 'c', 'd', 'e', 'f'
};

static int
p_seprint(Msg *m)
{
	int c, i, n, isstring;
	uchar *ps = m->ps;
	char *p = m->p;
	char *e = m->e;

	n = m->pe - ps;
	if(n > Nflag)
		n = Nflag;

	isstring = 1;
	for(i = 0; i < n; i++){
		c = ps[i];
		if(!isprint(c) && !isspace(c)){
			isstring = 0;
			break;
		}
	}

	if(isstring){
		for(i = 0; i < n && p+1<e; i++){
			c = ps[i];
			switch(c){
			case '\t':
				*p++ = '\\';
				*p++ = 't';
				break;
			case '\r':
				*p++ = '\\';
				*p++ = 'r';
				break;
			case '\n':
				*p++ = '\\';
				*p++ = 'n';
				break;
			default:
				*p++ = c;
			}
		}
	} else {
		for(i = 0; i < n && p+1<e; i++){
			c = ps[i];
			*p++ = tohex[c>>4];
			*p++ = tohex[c&0xf]; 
		}
	}

	m->pr = nil;
	m->p = p;
	m->ps = ps;

	return 0;
}

Proto dump =
{
	"dump",
	p_compile,
	nil,
	p_seprint,
	nil,
	nil,
	nil,
	defaultframer,
};
