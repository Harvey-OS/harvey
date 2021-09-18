#include "lib9.h"
#include <windows.h>


char*
getwd(char *buf, int size)
{
	int n;
	char *p;

	buf[0] = 0;

	n = GetCurrentDirectory(size, buf);
	if(n == 0) {
		/*winerror();*/
		return 0;
	}

	for(p=buf; *p; p++)
		if(*p == '\\')
			*p = '/';

	return buf;
}
