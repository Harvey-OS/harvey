#include <unistd.h>

extern char **environ;

int
execl(const char *name, const char *arg0, ...)
{
	return execve(name, &arg0, environ);
}
