#include <alef.h>
#include <bio.h>
#include "httpd.h"

/*
 * parse a search string of the form
 * tag=val&tag1=val1...
 */
Query*
parsequery(byte *search)
{
	Query *q;
	byte *tag, *val, *s;

	while((s = strchr(search, '?')) != nil)
		search = s + 1;
	q = nil;
	while(*search){
		tag = search;
		while(*search != '='){
			if(*search == 0)
				return q;
			search++;
		}
		*search++ = 0;
		val = search;
		while(*search != '&'){
			if(*search == 0)
				return mkquery(tag, val, q);
			search++;
		}
		*search++ = 0;
		q = mkquery(tag, val, q);
	}
	return q;
}

Query*
mkquery(byte *tag, byte *val, Query *next)
{
	Query *q;

	q = malloc(sizeof *q);
	q->tag = urlunesc(tag);
	q->val = urlunesc(val);
	q->next = next;
	return q;
}

void
freequery(Query *q)
{
	Query *next;

	if(q == nil)
		return;
	next = q->next;
	free(q);
	freequery(next);
}
