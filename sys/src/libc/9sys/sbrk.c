#include <u.h>
#include <libc.h>

extern	char	end[];
static	char	*bloc = { end };
extern	int	brk_(void*);

int
brk(void *p)
{
	ulong n;

	n = (ulong)p;
	n += 3;
	n &= ~3;
	if(brk_((void*)n) < 0)
		return -1;
	bloc = (char *)n;
	return 0;
}

void*
sbrk(ulong n)
{

	n += 3;
	n &= ~3;
	if(brk_((void*)(bloc+n)) < 0)
		return (void*)-1;
	bloc += n;
	return (void*)(bloc-n);
}
