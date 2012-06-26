#include <unistd.h>
#include <stdio.h>
#include <string.h>

char *
ctermid(char *s)
{
	static char buf[L_ctermid];

	if(s == 0)
		s = buf;
	strncpy(s, "/dev/cons", sizeof buf);
	return(s);
}

char *
ctermid_r(char *s)
{
	return s ? ctermid(s) : NULL;
}
