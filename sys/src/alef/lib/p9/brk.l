#include <alef.h>

extern	char	end[];
intern	char	*bloc;

int
brk(void *p)
{
	uint n;

	if(bloc == nil)
		bloc = end;
	n = (uint)p;
	n += 3;
	n &= ~3;
	if(brk_((void*)n) < 0)
		return -1;
	bloc = (char *)n;
	return 0;
}

void*
sbrk(uint n)
{
	if(bloc == nil)
		bloc = end;

	n += 3;
	n &= ~3;
	if(brk_((void*)(bloc+n)) < 0)
		return (void*)-1;
	bloc += n;

	return (void*)(bloc-n);
}
