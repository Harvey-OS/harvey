#include <unistd.h>

extern char **environ;

int
execv(const char *name, const char *argv[])
{
	return execve(name, argv, environ);
}
