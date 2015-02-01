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
#include "String.h"

#define isspace(c) ((c)==' ' || (c)=='\t' || (c)=='\n')

/* Get the next field from a String.  The field is delimited by white space,
 * single or double quotes.
 */
String *
s_parse(String *from, String *to)
{
	if (*from->ptr == '\0')
		return 0;
	if (to == 0)
		to = s_new();
	if (*from->ptr == '\'') {
		from->ptr++;
		for (;*from->ptr != '\'' && *from->ptr != '\0'; from->ptr++)
			s_putc(to, *from->ptr);
		if (*from->ptr == '\'')	
			from->ptr++;
	} else if (*from->ptr == '"') {
		from->ptr++;
		for (;*from->ptr != '"' && *from->ptr != '\0'; from->ptr++)
			s_putc(to, *from->ptr);
		if (*from->ptr == '"')	
			from->ptr++;
	} else {
		for (;!isspace(*from->ptr) && *from->ptr != '\0'; from->ptr++)
			s_putc(to, *from->ptr);
	}
	s_terminate(to);

	/* crunch trailing white */
	while(isspace(*from->ptr))
		from->ptr++;

	return to;
}
