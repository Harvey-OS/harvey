#include <u.h>
#include <libc.h>
#include <oventi.h>

enum {
	IdealAlignment = 32,
	ChunkSize 	= 128*1024,
};


void
vtMemFree(void *p)
{
	if(p == 0)
		return;
	free(p);
}


void *
vtMemAlloc(int size)
{
	void *p;

	p = malloc(size);
	if(p == 0)
		vtFatal("vtMemAlloc: out of memory");
	setmalloctag(p, getcallerpc(&size));
	return p;
}

void *
vtMemAllocZ(int size)
{
	void *p = vtMemAlloc(size);
	memset(p, 0, size);
	setmalloctag(p, getcallerpc(&size));
	return p;
}

void *
vtMemRealloc(void *p, int size)
{
	if(p == nil)
		return vtMemAlloc(size);
	p = realloc(p, size);
	if(p == 0)
		vtFatal("vtRealloc: out of memory");
	setrealloctag(p, getcallerpc(&size));
	return p;
}


void *
vtMemBrk(int n)
{
	static Lock lk;
	static uchar *buf;
	static int nbuf;
	static int nchunk;
	int align, pad;
	void *p;

	if(n >= IdealAlignment)
		align = IdealAlignment;
	else if(n > 8)
		align = 8;
	else	
		align = 4;

	lock(&lk);
	pad = (align - (uintptr)buf) & (align-1);
	if(n + pad > nbuf) {
		buf = vtMemAllocZ(ChunkSize);
		setmalloctag(buf, getcallerpc(&n));
		nbuf = ChunkSize;
		pad = (align - (uintptr)buf) & (align-1);
		nchunk++;
	}

	assert(n + pad <= nbuf);	
	
	p = buf + pad;
	buf += pad + n;
	nbuf -= pad + n;
	unlock(&lk);

	return p;
}

void
vtThreadSetName(char *name)
{
	int fd;
	char buf[32];

	sprint(buf, "/proc/%d/args", getpid());
	if((fd = open(buf, OWRITE)) >= 0){
		write(fd, name, strlen(name));
		close(fd);
	}
}

int
vtFdRead(int fd, uchar *buf, int n)
{
	n = read(fd, buf, n);
	if(n < 0) {
		vtOSError();
		return -1;
	}
	if(n == 0) {
		vtSetError("unexpected EOF");
		return 0;
	}
	return n;
}

int
vtFdWrite(int fd, uchar *buf, int n)
{
	int nn;
	
	nn = write(fd, buf, n);
	if(nn < 0) {
		vtOSError();
		return 0;
	}
	if(n != nn) {
		vtSetError("truncated write");
		return 0;
	}
	return 1;
}

void
vtFdClose(int fd)
{
	close(fd);
}

char *
vtOSError(void)
{
	return vtSetError("%r");
}
