#include	"u.h"
#include	"lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"

/*
 *  read in a segment
 */
static long
readseg(int dev, long (*read)(int, void*, int), long len, long addr)
{
	char *a;
	long n, sofar;

	a = (char *)addr;
	for(sofar = 0; sofar < len; sofar += n){
		n = 8*1024;
		if(len - sofar < n)
			n = len - sofar;
		n = (*read)(dev, a + sofar, n);
		if(n <= 0)
			break;
		print(".");
	}
	return sofar;
}

/*
 *  boot
 */
int
plan9boot(int dev, long (*seek)(int, long), long (*read)(int, void*, int))
{
	long n;
	long addr;
	void (*b)(void);
	Exec *ep;

	if((*seek)(dev, 0) < 0)
		return -1;

	/*
	 *  read header
	 */
	ep = (Exec *) ialloc(sizeof(Exec), 0);
	n = sizeof(Exec);
	if(readseg(dev, read, n, (ulong) ep) != n){
		print("premature EOF\n");
		return -1;
	}
	if(GLLONG(ep->magic) != I_MAGIC){
		print("bad magic 0x%lux not a plan 9 executable!\n", GLLONG(ep->magic));
		return -1;
	}

	/*
	 *  read text
	 */
	addr = PADDR(GLLONG(ep->entry));
	n = GLLONG(ep->text);
	print("%d", n);
	if(readseg(dev, read, n, addr) != n){
		print("premature EOF\n");
		return -1;
	}

	/*
	 *  read data (starts at first page after kernel)
	 */
	addr = PGROUND(addr+n);
	n = GLLONG(ep->data);
	print("+%d", n);
	if(readseg(dev, read, n, addr) != n){
		print("premature EOF\n");
		return -1;
	}

	/*
	 *  bss and entry point
	 */
	print("+%d\nstart at 0x%lux\n", GLLONG(ep->bss), GLLONG(ep->entry));

	/*
	 *  Go to new code. It's up to the program to get its PC relocated to
	 *  the right place.
	 */
	b = (void (*)(void))(PADDR(GLLONG(ep->entry)));
	(*b)();
	return 0;
}
