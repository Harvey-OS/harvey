#include <stdlib.h>

extern	char **environ;

char *
getenv(const char *name)
{
	char **p = environ;
	char *v, *s1, *s2;

	while (*p != NULL){
		for(s1 = (char *)name, s2 = *p++; *s1 == *s2; s1++, s2++)
			continue;
		if(*s1 == '\0' && *s2 == '=')
			return s2+1;
	}
	return NULL ;
}
