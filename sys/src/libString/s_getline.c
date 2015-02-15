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
#include <bio.h>
#include "String.h"

/* Append an input line to a String.
 *
 * Returns a pointer to the character string (or 0).
 * Leading whitespace and newlines are removed.
 *
 * Empty lines and lines starting with '#' are ignored.
 */ 
extern char *
s_getline(Biobuf *fp, String *to)
{
	int c;
	int len=0;

	s_terminate(to);

	/* end of input */
	if ((c = Bgetc(fp)) < 0)
		return 0;

	/* take care of inconsequentials */
	for(;;) {
		/* eat leading white */
		while(c==' ' || c=='\t' || c=='\n' || c=='\r')
			c = Bgetc(fp);

		if(c < 0)
			return 0;

		/* take care of comments */
		if(c == '#'){
			do {
				c = Bgetc(fp);
				if(c < 0)
					return 0;
			} while(c != '\n');
			continue;
		}

		/* if we got here, we've gotten something useful */
		break;
	}

	/* gather up a line */
	for(;;) {
		len++;
		switch(c) {
		case -1:
			s_terminate(to);
			return len ? to->ptr-len : 0;
		case '\\':
			c = Bgetc(fp);
			if (c != '\n') {
				s_putc(to, '\\');
				s_putc(to, c);
			}
			break;
		case '\n':
			s_terminate(to);
			return len ? to->ptr-len : 0;
		default:
			s_putc(to, c);
			break;
		}
		c = Bgetc(fp);
	}
}
