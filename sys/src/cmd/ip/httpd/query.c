#include <u.h>
#include <libc.h>
#include <bio.h>
#include "httpd.h"

/*
 * parse a search string of the form
 * tag=val&tag1=val1...
 */
SPairs*
parsequery(char *search)
{
	SPairs *q;
	char *tag, *val, *s;

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
				return mkspairs(urlunesc(tag), urlunesc(val), q);
			search++;
		}
		*search++ = 0;
		q = mkspairs(urlunesc(tag), urlunesc(val), q);
	}
	return q;
}
