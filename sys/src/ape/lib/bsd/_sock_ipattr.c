/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/* posix */
#include <sys/types.h>
#include <unistd.h>
#include <ctype.h>

/* bsd extensions */
#include <sys/uio.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "priv.h"

/*
 *  return ndb attribute type of an ip name
 */
int
_sock_ipattr(char *name)
{
	char *p;
	int dot = 0;
	int alpha = 0;

	for(p = name; *p; p++){
		if(isdigit(*p)){
			;
		}else if(isalpha(*p) || *p == '-')
			alpha = 1;
		else if(*p == '.')
			dot = 1;
		else
			return Tsys;
	}

	if(alpha){
		if(dot)
			return Tdom;
		else
			return Tsys;
	}

	if(dot)
		return Tip;
	else
		return Tsys;
}
