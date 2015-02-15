/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <stdio.h>
#include <pwd.h>
#include <stdlib.h>

static char PASSWD[]	= "/etc/passwd";
static FILE *pwf = NULL;
static char line[BUFSIZ+2];
static struct passwd passwd;

void
setpwent(void)
{
	if( pwf == NULL )
		pwf = fopen( PASSWD, "r" );
	else
		rewind( pwf );
}

void
endpwent(void)
{
	if( pwf != NULL ){
		fclose( pwf );
		pwf = NULL;
	}
}

static char *
pwskip(char *p)
{
	while( *p && *p != ':' && *p != '\n' )
		++p;
	if( *p ) *p++ = 0;
	else p[1] = 0;
	return(p);
}

struct passwd *
pwdecode(char *p)
{
	passwd.pw_name = p;
	p = pwskip(p);
	p = pwskip(p); /* passwd */
	passwd.pw_uid = atoi(p);
	p = pwskip(p);
	passwd.pw_gid = atoi(p);
	p = pwskip(p); /* comment */
	p = pwskip(p); /* gecos */
	passwd.pw_dir = p;
	p = pwskip(p);
	passwd.pw_shell = p;
	pwskip(p);
	return(&passwd);
}

struct passwd *
getpwent(void)
{
	register char *p;

	if (pwf == NULL) {
		if( (pwf = fopen( PASSWD, "r" )) == NULL )
			return(0);
	}
	p = fgets(line, BUFSIZ, pwf);
	if (p==NULL)
		return(0);
	return pwdecode (p);
}
