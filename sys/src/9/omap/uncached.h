/*
 * running the l2 cache as write-back and using cached memory for
 * usb data structures yields spurious errors such as
 *
 *	qhintr: td 0x60ee3d80 csw 0x8824a error 0x48 transaction error
 *
 * from usbehci.  so, at least for now, we will use uncached memory until
 * we sort out the write-back problems.
 */
#define free			ucfree
#define malloc			myucalloc
#define mallocz			ucallocz
#define smalloc			myucalloc
#define xspanalloc		ucallocalign

#define allocb			ucallocb
#define iallocb			uciallocb
#define freeb			ucfreeb

static void *
ucallocz(uint n, int)
{
	char *p = ucalloc(n);

	if (p)
		memset(p, 0, n);
	else
		panic("ucalloc: out of memory");
	return p;
}

static void *
myucalloc(uint n)
{
	return ucallocz(n, 1);
}
