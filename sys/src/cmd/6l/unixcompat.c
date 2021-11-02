#include	"l.h"

/*
 * fake malloc
 */
void*
malloc(ulong n)
{
	void *p;

	while(n & 7)
		n++;
	while(nhunk < n)
		gethunk();
	p = hunk;
	nhunk -= n;
	hunk += n;
	return p;
}

void
free(void *p)
{
	USED(p);
}

void*
calloc(ulong m, ulong n)
{
	void *p;

	n *= m;
	p = malloc(n);
	memset(p, 0, n);
	return p;
}

void*
realloc(void*_, ulong __)
{
	fprint(2, "realloc called\n");
	abort();
	return 0;
}

void*
mysbrk(ulong size)
{
	return sbrk(size);
}

void
setmalloctag(void *v, ulong pc)
{
	USED(v);
	USED(pc);
}

int
fileexists(char *s)
{
	uchar dirbuf[400];
	int stat(const char *pathname, void *_/*struct stat *statbuf*/);

	/* it's fine if stat result doesn't fit in dirbuf, since even then the file exists */
	return stat(s, dirbuf) >= 0;
}

int
myaccess(char *f)
{
	return access(f, AEXIST);
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

// things are not always included? What? 
int
systemtype(int sys)
{
	// Stuff scattered every damn where.
	enum                            /* also in ../{8a,0a}.h */
	{
        Plan9   = 1<<0,
        Unix    = 1<<1,
        Windows = 1<<2,
	};
	switch (sys) {
		case Plan9:
		case Unix:
			return 1;
		default: 
			return 0;
	}
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
