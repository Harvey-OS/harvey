#include <alef.h>

byte*
strdup(byte *s) 
{  
	byte *os;

	os = malloc(strlen(s) + 1);
	if(os)
		strcpy(os, s);

	return os;

}
