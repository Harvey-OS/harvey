#include	"cc.h"

void*
mysbrk(ulong size)
{
	return sbrk(size);
}

int
mycreat(char *n, int p)
{

	return create(n, 1, p);
}

int
mywait(int *s)
{
	int p;
	Waitmsg *w;

	if((w = wait()) == nil)
		return -1;
	else{
		p = w->pid;
		*s = 0;
		if(w->msg[0])
			*s = 1;
		free(w);
		return p;
	}
}

int
mydup(int f1, int f2)
{
	return dup(f1,f2);
}

int
mypipe(int *fd)
{
	return pipe(fd);
}

int
systemtype(int sys)
{

	return sys&Plan9;
}

int
pathchar(void)
{
	return '/';
}

char*
mygetwd(char *path, int len)
{
	return getwd(path, len);
}

int
myexec(char *path, char *argv[])
{
	return exec(path, argv);
}

int
myfork(void)
{
	return fork();
}

/*
 * fake mallocs
 */
void*
malloc(ulong n)
{
	return alloc(n);
}

void*
calloc(ulong m, ulong n)
{
	return alloc(m*n);
}

void*
realloc(void*, ulong)
{
	fprint(2, "realloc called\n");
	abort();
	return 0;
}

void
free(void*)
{
}

int
myaccess(char *f)
{
	return access(f, AEXIST);
}

void
setmalloctag(void*, ulong)
{
}
