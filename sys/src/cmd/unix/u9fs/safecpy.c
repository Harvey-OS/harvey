#include <stdio.h>

void
safecpy(char *to, char *from, int tolen)
{
	int fromlen;
	memset(to, 0, tolen);
	fromlen = from ? strlen(from) : 0;
	if (fromlen > tolen)
		fromlen = tolen;
	memcpy(to, from, fromlen);
}
