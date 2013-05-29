#include <unistd.h>

int
execle(const char *name, const char *arg0, const char *, ...)
{
	char *p;

	for(p=(char *)(&name)+1; *p; )
		p++;
	return execve(name, &arg0, (char **)p+1);
}
