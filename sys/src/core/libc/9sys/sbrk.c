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
	uintptr bl;

	bl = ((uintptr)p + Round) & ~Round;
	if(brk_((void*)bl) < 0)
		return -1;
	bloc = (char*)bl;
	return 0;
}

void*
sbrk(ulong n)
{
	uintptr bl;

	bl = ((uintptr)bloc + Round) & ~Round;
	if(brk_((void*)(bl+n)) < 0)
		return (void*)-1;
	bloc = (char*)bl + n;
	return (void*)bl;
}
