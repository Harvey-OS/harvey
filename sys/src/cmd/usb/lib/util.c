#include <u.h>
#include <libc.h>
#include <thread.h>
#include "usb.h"

int debug, debugdebug, verbose;

char *
namefor(Namelist *list, int item)
{
	while (list->name){
		if (list->index == item)
			return list->name;
		list++;
	}
	return "<unnamed>";
}

void *
emalloc(ulong size)
{
	void *x;

	x = malloc(size);
	if (x == nil)
		sysfatal("maloc: %r");
	return x;
}

void *
emallocz(ulong size, int zero)
{
	void *x;

	x = malloc(size);
	if (x == nil)
		sysfatal("maloc: %r");
	if (zero)
		memset(x, 0, size);
	return x;
}
