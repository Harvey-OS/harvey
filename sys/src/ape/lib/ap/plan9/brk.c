#include "lib.h"
#include <inttypes.h>
#include <errno.h>
#include "sys9.h"

char	end[];
static	char	*bloc = { end };
extern	int	_BRK_(void*);

char *
brk(char *p)
{
	uintptr_t n;

	n = (uintptr_t)p;
	n += sizeof(uintptr_t) - 1;
	n &= ~(sizeof(uintptr_t) - 1);
	if(_BRK_((void*)n) < 0){
		errno = ENOMEM;
		return (char *)-1;
	}
	bloc = (char *)n;
	return 0;
}

void *
sbrk(uintptr_t n)
{
	n += sizeof(uintptr_t) - 1;
	n &= ~(sizeof(uintptr_t) - 1);
	if(_BRK_((void *)(bloc+n)) < 0){
		errno = ENOMEM;
		return (void *)-1;
	}
	bloc += n;
	return (void *)(bloc-n);
}
