#include	"cc.h"
#include	<sys/wait.h>

void*
mysbrk(ulong size)
{
	return (void*)sbrk(size);
}

int
mycreat(char *n, int p)
{

	return create(n, 1, p);
}

int
mywait(int *s)
{
	return wait(s);
}

int
mydup(int f1, int f2)
{
	if(f2 < 0)
		return dup(f1);
	return dup2(f1,f2);
}

int
mypipe(int *fd)
{
	return pipe(fd);
}

int
systemtype(int sys)
{

	return sys&Unix;
}

int
pathchar(void)
{
	return '/';
}

char*
mygetwd(char *path, int len)
{
	return (char*)getcwd(path, len);
}

int
myexec(char *path, char *argv[])
{
	return execvp(path, argv);
}

/*
 * fake mallocs
 */
void*
malloc(size_t n)
{
	return alloc(n);
}

void*
calloc(size_t m, size_t n)
{
	return alloc(m*n);
}

void*
realloc(void *p, size_t n)
{
	fprint(2, "realloc called\n");
	abort();
	return 0;
}

void
free(void *p)
{
}

int
myfork(void)
{
	return fork();
}

int
myaccess(char *f)
{
	return access(f, 0);
}
