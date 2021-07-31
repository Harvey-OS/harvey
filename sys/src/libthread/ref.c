#include <u.h>
#include <libc.h>
#include <threadimpl.h>

void
incref(Ref *r)
{
	// returns the old value;
	_xinc(&r->ref);
}

long
decref(Ref *r)
{
	// returns the new value;
	return _xdec(&r->ref);
}
