/*
 * Stub.
 */
#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "../port/error.h"

extern Dev* devtab[];

void
devtabreset(void)
{
	int i;

	for(i = 0; devtab[i] != nil; i++) {
		if (devtab[i]->reset == nil)
			panic("corrupt memory: nil devtab[%d]->reset", i);
		devtab[i]->reset();
	}
}

void
devtabinit(void)
{
	int i;

	for(i = 0; devtab[i] != nil; i++)
		devtab[i]->init();
}

void
devtabshutdown(void)
{
	int i;

	/*
	 * Shutdown in reverse order.
	 */
	for(i = 0; devtab[i] != nil; i++)
		;
	for(i--; i >= 0; i--)
		devtab[i]->shutdown();
}
