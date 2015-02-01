/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <stdio.h>
#include <grp.h>
#include <stdlib.h>

#define	CL	':'
#define	CM	','
#define	NL	'\n'
#define	MAXGRP	100

static char GROUP[] = "/etc/group";
static FILE *grf = NULL;
static char line[BUFSIZ+1];
static struct group group;
static char *gr_mem[MAXGRP];

void
setgrent(void)
{
	if( !grf )
		grf = fopen( GROUP, "r" );
	else
		rewind( grf );
}

void
endgrent(void)
{
	if( grf ){
		fclose( grf );
		grf = NULL;
	}
}

static char *
grskip(register char *p, register c)
{
	while( *p && *p != c ) ++p;
	if( *p ) *p++ = 0;
	return( p );
}

struct group *
getgrent()
{
	register char *p, **q;

	if( !grf && !(grf = fopen( GROUP, "r" )) )
		return(NULL);
	if( !(p = fgets( line, BUFSIZ, grf )) )
		return(NULL);
	group.gr_name = p;
	p = grskip(p,CL); /* passwd */
	group.gr_gid = atoi(p = grskip(p,CL));
	group.gr_mem = gr_mem;
	p = grskip(p,CL);
	grskip(p,NL);
	q = gr_mem;
	while( *p ){
		*q++ = p;
		p = grskip(p,CM);
	}
	*q = NULL;
	return( &group );
}
