#include <unistd.h>
#include <sys/limits.h>
#include <string.h>

extern char **environ;

/*
 * BUG: instead of looking at PATH env variable,
 * just try prepending /bin/ if name fails...
 */

int
execvp(const char *name, const char **argv)
{
	int n;
	char buf[PATH_MAX];

	if((n=execve(name, argv, environ)) < 0){
		strcpy(buf, "/bin/");
		strcpy(buf+5, name);
		n = execve(buf, argv, environ);
	}
	return n;
}
