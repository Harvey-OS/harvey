#include <string.h>

char*
strcat(char *s1, const char *s2)
{

	strcpy(strchr(s1, 0), s2);
	return s1;
}
