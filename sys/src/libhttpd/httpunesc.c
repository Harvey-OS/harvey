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
#include <bin.h>
#include <httpd.h>

/*
 *  go from http with latin1 escapes to utf,
 *  we assume that anything >= Runeself is already in utf
 */
char *
httpunesc(HConnect *cc, char *s)
{
	char *t, *v;
	int c;
	Htmlesc *e;

	v = halloc(cc, UTFmax*strlen(s) + 1);
	for(t = v; c = *s;){
		if(c == '&'){
			if(s[1] == '#' && s[2] && s[3] && s[4] && s[5] == ';'){
				c = atoi(s+2);
				if(c < Runeself){
					*t++ = c;
					s += 6;
					continue;
				}
				if(c < 256 && c >= 161){
					e = &htmlesc[c-161];
					t += runetochar(t, &e->value);
					s += 6;
					continue;
				}
			} else {
				for(e = htmlesc; e->name != nil; e++)
					if(strncmp(e->name, s, strlen(e->name)) == 0)
						break;
				if(e->name != nil){
					t += runetochar(t, &e->value);
					s += strlen(e->name);
					continue;
				}
			}
		}
		*t++ = c;
		s++;
	}
	*t = 0;
	return v;
}
