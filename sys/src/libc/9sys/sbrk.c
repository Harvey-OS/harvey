#include <u.h>
#include <libc.h>

extern	char	end[];
static	char	*bloc = { end };
extern	int	brk_(void*);

enum
{
	Round	= 7
};

int
brk(void *p)
{
	void * bl;

	bl = (void *)(((uintptr)p + Round) & ~Round);
	if(brk_(bl) < 0)
		return -1;
	bloc = bl;
	return 0;
}

void*
sbrk(ulong n)
{
	char *bl, *nb;

	bl = bloc;
	nb = (char *)((((uintptr)bl + n) + Round) & ~Round);
	if(brk_(nb) < 0)
		return (void*)-1;
	bloc = nb;
	return bl;
}
