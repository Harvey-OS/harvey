#include <u.h>
#include <libc.h>
#include <bio.h>
#include <ctype.h>
#include <ndb.h>
#include "ndbhf.h"


/*
 *  parse a single tuple
 */
char*
_ndbparsetuple(char *cp, Ndbtuple **tp)
{
	char *p;
	int len;
	Ndbtuple *t;

	/* a '#' starts a comment lasting till new line */
	EATWHITE(cp);
	if(*cp == '#' || *cp == '\n')
		return 0;

	/* we keep our own free list to reduce mallocs */
	t = malloc(sizeof(Ndbtuple));
	if(t == 0)
		return 0;
	memset(t, 0, sizeof(*t));
	*tp = t;

	/* parse attribute */
	p = cp;
	while(*cp != '=' && !ISWHITE(*cp) && *cp != '\n')
		cp++;
	len = cp - p;
	if(len >= Ndbalen)
		len = Ndbalen;
	strncpy(t->attr, p, len);

	/* parse value */
	EATWHITE(cp);
	if(*cp == '='){
		cp++;
		EATWHITE(cp);
		if(*cp == '"'){
			p = ++cp;
			while(*cp != '\n' && *cp != '"')
				cp++;
			len = cp - p;
			if(*cp == '"')
				cp++;
		} else {
			p = cp;
			while(!ISWHITE(*cp) && *cp != '\n')
				cp++;
			len = cp - p;
		}
		if(len >= Ndbvlen)
			len = Ndbvlen;
		strncpy(t->val, p, len);
	}

	return cp;
}

/*
 *  parse all tuples in a line.  we assume that the 
 *  line ends in a '\n'.
 *
 *  the tuples are linked as a list using ->entry and
 *  as a ring using ->line.
 */
Ndbtuple*
_ndbparseline(char *cp)
{
	Ndbtuple *t;
	Ndbtuple *first, *last;

	first = last = 0;
	while(*cp != '#' && *cp != '\n'){
		t = 0;
		cp = _ndbparsetuple(cp, &t);
		if(cp == 0)
			break;
		if(first){
			last->line = t;
			last->entry = t;
		} else
			first = t;
		last = t;
		t->line = 0;
		t->entry = 0;
	}
	if(first)
		last->line = first;
	return first;
}
