#include <u.h>
#include <libc.h>
#include <thread.h>
#include "threadimpl.h"

void
incref(Ref *r)
{
	ainc(&r->ref);
}

long
decref(Ref *r)
{
	return adec(&r->ref);
}
