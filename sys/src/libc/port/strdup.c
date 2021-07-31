#include <u.h>
#include <libc.h>

char*
strdup(char *s) 
{  
	char *ns;

	ns = malloc(strlen(s) + 1);
	if(ns == 0)
		return 0;

	return strcpy(ns, s);
}
