#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"

/*
 *  parse a command written to a device
 */
Cmdbuf*
parsecmd(char *p, int n)
{
	Cmdbuf *cb;

	cb = smalloc(sizeof(*cb));
	
	if(n > sizeof(cb->buf)-1)
		n = sizeof(cb->buf)-1;
	memmove(cb->buf, p, n);
	if(n > 0 && cb->buf[n-1] == '\n')
		n--;
	cb->buf[n] = '\0';
	cb->nf = getfields(cb->buf, cb->f, nelem(cb->f), 1, " ");
	return cb;
}

