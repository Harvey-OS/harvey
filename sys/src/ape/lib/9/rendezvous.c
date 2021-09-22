#include <lib9.h>

extern	void*	_RENDEZVOUS(void *, void *);

void*
rendezvous(void * tag, void * value)
{
	return _RENDEZVOUS(tag, value);
}
