#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "regexp.h"

void
regerror(char *s)
{
	char buf[132];

	strcpy(buf, "regerror: ");
	strcat(buf, s);
	strcat(buf, "\n");
	fwrite(buf, 1, strlen(buf), stderr);
	exit(1);
}
