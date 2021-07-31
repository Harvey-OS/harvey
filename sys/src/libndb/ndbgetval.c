#include <u.h>
#include <libc.h>
#include <bio.h>
#include "ndb.h"

/*
 *  search for a tuple that has the given 'attr=val' and also 'rattr=x'.
 *  copy 'x' into 'buf' and return the whole tuple.
 *
 *  return 0 if not found.
 */
Ndbtuple*
ndbgetval(Ndb *db, Ndbs *s, char *attr, char *val, char *rattr, char *buf)
{
	Ndbtuple *t, *nt;

	t = ndbsearch(db, s, attr, val);
	while(t){
		/* first look on same line (closer binding) */
		nt = s->t;
		for(;;){
			if(strcmp(rattr, nt->attr) == 0){
				strncpy(buf, nt->val, Ndbvlen);
				return t;
			}
			nt = nt->line;
			if(nt == s->t)
				break;
		}
		/* search whole tuple */
		for(nt = t; nt; nt = nt->entry)
			if(strcmp(rattr, nt->attr) == 0){
				strncpy(buf, nt->val, Ndbvlen);
				return t;
			}
		ndbfree(t);
		t = ndbsnext(s, attr, val);
	}
	return 0;
}
