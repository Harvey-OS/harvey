#include <windows.h>
#include <lib9.h>

/*
 *	We can't include l.h, because Windoze wants to use some names
 *	like FLOAT and ABC which we declare.  Define what we need here.
 */
typedef	unsigned char	uchar;
typedef	unsigned int	uint;
typedef	unsigned long	ulong;

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

double
cputime(void)
{
	return 0.0;
}

int
fileexists(char *name)
{
	int fd;

	fd = open(name, OREAD);
	if(fd < 0)
		return 0;
	close(fd);
	return 1;
}
