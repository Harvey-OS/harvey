#include <u.h>
#include <libc.h>
#include <bio.h>
#include <ctype.h>
#include <ndb.h>
#include "ndbhf.h"

/*
 *  free a parsed entry
 */
void
ndbfree(Ndbtuple *t)
{
	Ndbtuple *tn;

	for(; t; t = tn){
		tn = t->entry;
		free(t);
	}
}
