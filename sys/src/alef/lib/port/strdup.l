#include <alef.h>

char*
strdup(char *s) 
{  
	char *os;

	os = malloc(strlen(s) + 1);
	if(os)
		strcpy(os, s);

	return os;

}
