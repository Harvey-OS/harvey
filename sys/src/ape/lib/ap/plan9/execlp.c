#include <unistd.h>
#include <string.h>
#include <sys/limits.h>

/*
 * BUG: instead of looking at PATH env variable,
 * just try prepending /bin/ if name fails...
 */

extern char **environ;

int
execlp(const char *name, const char *arg0, ...)
{
	int n;
	char buf[PATH_MAX];

	if((n=execve(name, &arg0, environ)) < 0){
		strcpy(buf, "/bin/");
		strcpy(buf+5, name);
		n = execve(buf, &name+1, environ);
	}
	return n;
}
