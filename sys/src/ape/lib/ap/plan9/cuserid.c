#include <unistd.h>
#include <stdio.h>
#include <string.h>

/*
 * BUG: supposed to be for effective uid,
 * but plan9 doesn't have that concept
 */
char *
cuserid(char *s)
{
	char *logname;
	static char buf[L_cuserid];

	if((logname = getlogin()) == NULL)
		return(NULL);
	if(s == 0)
		s = buf;
	strncpy(s, logname, sizeof buf);
	return(s);
}
