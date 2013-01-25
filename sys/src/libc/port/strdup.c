#include <u.h>
#include <libc.h>

char*
strdup(char *s) 
{  
	char *ns;

	ns = malloc(strlen(s) + 1);
	if(ns == 0)
		return 0;
	setmalloctag(ns, getcallerpc(&s));

	return strcpy(ns, s);
}
