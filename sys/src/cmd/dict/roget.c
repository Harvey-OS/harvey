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
#include <ctype.h>
#include "dict.h"

/* Roget's Thesaurus from project Gutenberg */

static int32_t Last = 0;

void
rogetprintentry(Entry e, int cmd)
{
	int spc;
	char c, *p;

	spc = 0;
	p = e.start;

	if(cmd == 'h'){
		while(!isspace(*p) && p < e.end)
			p++;
		while(strncmp(p, " -- ", 4) != 0 && p < e.end){
			while(isspace(*p) && p < e.end)
				p++;
			if (*p == '[' || *p == '{'){
				c = (*p == '[')? ']': '}';
				while(*p != c && p < e.end)
					p++;
				p++;
				continue;
			}
			if (isdigit(*p) || ispunct(*p)){
				while(!isspace(*p) && p < e.end)
					p++;
				continue;
			}


			if (isspace(*p))
				spc = 1;
			else
			if (spc){
				outchar(' ');
				spc = 0;
			}

			while(!isspace(*p) && p < e.end)
				outchar(*p++);
		}
		return;
	}

	while(p < e.end && !isspace(*p))
		p++;
	while(p < e.end && isspace(*p))
		p++;

	while (p < e.end){
		if (p < e.end -4 && strncmp(p, " -- ", 4) == 0){	/* first line */
			outnl(2);
			p += 4;
			spc = 0;
		}

		if (p < e.end -2 && strncmp(p, "[ ", 4) == 0){		/* twiddle layout */
			outchars(" [");
			continue;
		}

		if (p < e.end -4 && strncmp(p, "&c (", 4) == 0){	/* usefull xref */
			if (spc)
				outchar(' ');
			outchar('/');
			while(p < e.end && *p != '(')
				p++;
			p++;
			while(p < e.end && *p != ')')
				outchar(*p++);
			p++;
			while(p < e.end && isspace(*p))
				p++;
			while(p < e.end && isdigit(*p))
				p++;
			outchar('/');
			continue;
		}

		if (p < e.end -3 && strncmp(p, "&c ", 3) == 0){		/* less usefull xref */
			while(p < e.end && !isdigit(*p))
				p++;
			while(p < e.end && isdigit(*p))
				p++;
			continue;
		}

		if (*p == '\n' && p < (e.end -1)){			/* their newlines */
			spc = 0;
			p++;
			if (isspace(*p)){				/* their continuation line */
				while (isspace(*p))
					p++;
				p--;
			}
			else{
				outnl(2);
			}
		}
		if (spc && *p != ';' && *p != '.' &&
		    *p != ',' && !isspace(*p)){				/* drop spaces before punct */
			spc = 0;
			outchar(' ');
		}
		if (isspace(*p))
			spc = 1;
		else
			outchar(*p);
		p++;
	}
	outnl(0);
}

int32_t
rogetnextoff(int32_t fromoff)
{
	int i;
	int64_t l;
	char *p;

	Bseek(bdict, fromoff, 0);
	Brdline(bdict, '\n');
	while ((p = Brdline(bdict, '\n')) != nil){
		l = Blinelen(bdict);
		if (!isdigit(*p))
			continue;
		for (i = 0; i < l-4; i++)
			if (strncmp(p+i, " -- ", 4) == 0)
				return Boffset(bdict)-l;
	}
	return Boffset(bdict);
}

void
rogetprintkey(void)
{
	Bprint(bout, "No pronunciation key.\n");
}
