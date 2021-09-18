#include	<windows.h>  
#include	<lib9.h>

#define	Windows	(1<<2)		/* hack - can't include cc.h because of clashes */
#define	Chunk	(1*1024*1024)

void*
mysbrk(ulong size)
{
	void *v;
	static int chunk;
	static uchar *brk;

	if(chunk < size) {
		chunk = Chunk;
		if(chunk < size)
			chunk = Chunk + size;
		brk = VirtualAlloc(NULL, chunk, MEM_COMMIT, PAGE_EXECUTE_READWRITE); 	
		if(brk == 0)
			return (void*)-1;
	}
	v = brk;
	chunk -= size;
	brk += size;
	return v;
}

int
mycreat(char *n, int p)
{

	return create(n, 1, p);
}

int
mywait(int *s)
{
	fprint(2, "mywait called\n");
	abort();
	return 0;
}

int
mydup(int f1, int f2)
{
	fprint(2, "mydup called\n");
	abort();
	return 0;
}

int
mypipe(int *fd)
{
	fprint(2, "mypipe called\n");
	abort();
	return 0;
}

int
systemtype(int sys)
{

	return sys&Windows;
}

int
pathchar(void)
{
	return '/';
}

char*
mygetwd(char *path, int len)
{
	return getcwd(path, len);
}

int
myexec(char *path, char *argv[])
{
	fprint(2, "myexec called\n");
	abort();
	return 0;
}

int
myfork(void)
{
	fprint(2, "myfork called\n");
	abort();
	return 0;
}

/*
 * fake mallocs
 */
void*
malloc(uint n)
{
	return mysbrk(n);
}

void*
calloc(uint m, uint n)
{
	return mysbrk(m*n);
}

void*
realloc(void *p, uint n)
{
	void *new;

	new = malloc(n);
	if(new && p)
		memmove(new, p, n);
	return new;
}

void
free(void *p)
{
}

int
myaccess(char *f)
{
	int fd;

	fd = open(f, OREAD);
	if(fd < 0)
		return -1;
	close(fd);
	return 0;
}
