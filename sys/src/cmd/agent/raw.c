#include "agent.h"

void*
rawopen(Key *k, char*, int)
{
	return k;
}

int
rawread(void *v, void *buf, int n)
{
	Key *k;

	k = v;
	if(n > s_len(k->data))
		n = s_len(k->data);
	memmove(buf, s_to_c(k->data), n);
	return n;
}

int
rawwrite(void*, void*, int)
{
	werrstr("not allowed");
	return -1;
}

Proto raw = {
.name=	"raw",
.perm=	0444,
.open=	rawopen,
.write=	rawwrite,
.read=	rawread,
};

