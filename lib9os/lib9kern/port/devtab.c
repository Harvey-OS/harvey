#include	"dat.h"
#include	"fns.h"
#include	"error.h"

extern Dev* devtab[];

long
devtabread(Chan *c, void* buf, long n, vlong off)
{
	int i;
	Dev *dev;
	char *alloc, *e, *p;

	USED(c);
	alloc = malloc(READSTR);
	if(alloc == nil)
		p9error(Enomem);

	p = alloc;
	e = p + READSTR;
	for(i = 0; devtab[i] != nil; i++){
		dev = devtab[i];
		p = seprint(p, e, "#%C %s\n", dev->dc, dev->name);
	}

	if(waserror()){
		free(alloc);
		nexterror();
	}
	n = readstr(off, buf, n, alloc);
	free(alloc);
	poperror();

	return n;
}
