#include <lib9.h>

extern	unsigned long	_RENDEZVOUS(unsigned long, unsigned long);

unsigned long
rendezvous(unsigned long tag, unsigned long value)
{
	return _RENDEZVOUS(tag, value);
}
