#include <u.h>
#include <libc.h>
#include "regexp.h"

void
regerror(char *s)
{
	char buf[132];

	strcpy(buf, "regerror: ");
	strcat(buf, s);
	strcat(buf, "\n");
	write(2, buf, strlen(buf));
	exits("regerr");
}
