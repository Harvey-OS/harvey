#include <u.h>
#include <libc.h>
#include <ip.h>
#include <thread.h>
#include "netbios.h"

void *
nbemalloc(ulong nbytes)
{
	void *p;
	p = malloc(nbytes);
	if (p == nil) {
		print("nbemalloc: failed\n");
		threadexitsall("mem");
	}
	return p;
}
