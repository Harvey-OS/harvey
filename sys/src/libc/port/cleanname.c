#include <u.h>
#include <libc.h>

/*
 * In place, rewrite name to compress multiple /, eliminate ., and process ..
 */
#define SEP(x)	((x)=='/' || (x) == 0)
char*
cleanname(char *name)
{
	char *p, *q, *dotdot;
	int rooted, erasedprefix;

	rooted = name[0] == '/';
	erasedprefix = 0;

	/*
	 * invariants:
	 *	p points at beginning of path element we're considering.
	 *	q points just past the last path element we wrote (no slash).
	 *	dotdot points just past the point where .. cannot backtrack
	 *		any further (no slash).
	 */
	p = q = dotdot = name+rooted;
	while(*p) {
		if(p[0] == '/')	/* null element */
			p++;
		else if(p[0] == '.' && SEP(p[1])) {
			if(p == name)
				erasedprefix = 1;
			p += 1;	/* don't count the separator in case it is nul */
		} else if(p[0] == '.' && p[1] == '.' && SEP(p[2])) {
			p += 2;
			if(q > dotdot) {	/* can backtrack */
				while(--q > dotdot && *q != '/')
					;
			} else if(!rooted) {	/* /.. is / but ./../ is .. */
				if(q != name)
					*q++ = '/';
				*q++ = '.';
				*q++ = '.';
				dotdot = q;
			}
			if(q == name)
				erasedprefix = 1;	/* erased entire path via dotdot */
		} else {	/* real path element */
			if(q != name+rooted)
				*q++ = '/';
			while((*q = *p) != '/' && *q != 0)
				p++, q++;
		}
	}
	if(q == name)	/* empty string is really ``.'' */
		*q++ = '.';
	*q = '\0';
	if(erasedprefix && name[0] == '#'){	
		/* this was not a #x device path originally - make it not one now */
		memmove(name+2, name, strlen(name)+1);
		name[0] = '.';
		name[1] = '/';
	}
	return name;
}
