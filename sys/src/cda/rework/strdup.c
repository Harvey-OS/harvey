#include "rework.h"

char *
strdup(char *s)
{
	char *t;

	t = (char *)malloc(strlen(s) + 1);
	return strcpy(t, s);
}
