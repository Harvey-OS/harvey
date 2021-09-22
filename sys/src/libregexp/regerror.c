#include <u.h>
#include <libc.h>
#include "regexp.h"

void
regerror(char *s)
{
	int len, slen;
	char buf[132];

	strcpy(buf, "regerror: ");
	len = strlen(buf) + 1;
	slen = strlen(s);
	if (len + slen + 1 > sizeof buf)
		slen = sizeof buf - len - 1;	/* truncate s */
	strncat(buf, s, slen);
	strcat(buf, "\n");
	write(2, buf, strlen(buf));
	exits("regerr");
}
