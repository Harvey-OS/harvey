#include <u.h>
#include <libc.h>
#include <fcall.h>
#include <thread.h>
#include <9p.h>

static void
rforker(void (*fn)(void*), void *arg, int flag)
{
	switch(rfork(RFPROC|RFMEM|RFNOWAIT|flag)){
	case -1:
		sysfatal("rfork: %r");
	default:
		return;
	case 0:
		fn(arg);
		_exits(0);
	}
}

void
listensrv(Srv *s, char *addr)
{
	_forker = rforker;
	_listensrv(s, addr);
}

void
postmountsrv(Srv *s, char *name, char *mtpt, int flag)
{
	_forker = rforker;
	_postmountsrv(s, name, mtpt, flag);
}

