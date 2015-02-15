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
#include <httpd.h>

/*
 * parse a search string of the form
 * tag=val&tag1=val1...
 */
HSPairs*
hparsequery(HConnect *c, char *search)
{
	HSPairs *q;
	char *tag, *val, *s;

	while((s = strchr(search, '?')) != nil)
		search = s + 1;
	s = search;
	while((s = strchr(s, '+')) != nil)
		*s++ = ' ';
	q = nil;
	while(*search){
		tag = search;
		while(*search != '='){
			if(*search == '\0')
				return q;
			search++;
		}
		*search++ = 0;
		val = search;
		while(*search != '&'){
			if(*search == '\0')
				return hmkspairs(c, hurlunesc(c, tag), hurlunesc(c, val), q);
			search++;
		}
		*search++ = '\0';
		q = hmkspairs(c, hurlunesc(c, tag), hurlunesc(c, val), q);
	}
	return q;
}
