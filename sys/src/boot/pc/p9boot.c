#include	"u.h"
#include	"lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"

/*
 *  read in a segment
 */
long
p9readseg(int dev, long (*read)(int, void*, int), long len, long addr)
{
	char *a;
	long n, sofar;

	a = (char *)addr;
	for(sofar = 0; sofar < len; sofar += n){
		n = 8*1024;
		if(len - sofar < n)
			n = len - sofar;
		n = (*read)(dev, a + sofar, n);
		if(n < 0)
			break;
		print(".");
	}
	return sofar;
}

#define I_MAGIC		((((4*11)+0)*11)+7)

typedef struct Exec Exec;
struct	Exec
{
	uchar	magic[4];		/* magic number */
	uchar	text[4];	 	/* size of text segment */
	uchar	data[4];	 	/* size of initialized data */
	uchar	bss[4];	  		/* size of uninitialized data */
	uchar	syms[4];	 	/* size of symbol table */
	uchar	entry[4];	 	/* entry point */
	uchar	spsz[4];		/* size of sp/pc offset table */
	uchar	pcsz[4];		/* size of pc/line number table */
};

/*
 *  boot
 */
int
p9boot(int dev, long (*seek)(int, long), long (*read)(int, void*, int))
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
	addr = BY2PG;
	n = sizeof(Exec);
	if(p9readseg(dev, read, n, addr) != n){
		print("premature EOF\n");
		return -1;
	}
	ep = (Exec *)BY2PG;
	if(GLLONG(ep->magic) != I_MAGIC){
		print("bad magic 0x%lux not a plan 9 executable!\n", GLLONG(ep->magic));
		return -1;
	}

	/*
	 *  read text (starts at second page)
	 */
	addr += sizeof(Exec);
	n = GLLONG(ep->text);
	print("%d", n);
	if(p9readseg(dev, read, n, addr) != n){
		print("premature EOF\n");
		return -1;
	}

	/*
	 *  read data (starts at first page after kernel)
	 */
	addr = PGROUND(addr+n);
	n = GLLONG(ep->data);
	print("+%d", n);
	if(p9readseg(dev, read, n, addr) != n){
		print("premature EOF\n");
		return -1;
	}

	/*
	 *  bss and entry point
	 */
	print("+%d start at 0x%lux\n", GLLONG(ep->bss), GLLONG(ep->entry));

	/*
	 *  Go to new code.  To avoid assumptions about where the program
	 *  thinks it is mapped, mask off the high part of the entry
	 *  address.  It's up to the program to get it's PC relocated to
	 *  the right place.
	 */
	b = (void (*)(void))(GLLONG(ep->entry) & 0xffff);
	(*b)();
	return 0;
}
