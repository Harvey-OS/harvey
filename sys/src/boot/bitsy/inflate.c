#include "u.h"
#include "lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#include <flate.h>

typedef struct Biobuf	Biobuf;

struct Biobuf
{
	uchar *bp;
	uchar *p;
	uchar *ep;
};

static int	header(Biobuf*);
static int	trailer(Biobuf*, Biobuf*);
static int	getc(void*);
static ulong	offset(Biobuf*);
static int	crcwrite(void *out, void *buf, int n);
static ulong	get4(Biobuf *b);
static ulong	Boffset(Biobuf *bp);

/* GZIP flags */
enum {
	Ftext=		(1<<0),
	Fhcrc=		(1<<1),
	Fextra=		(1<<2),
	Fname=		(1<<3),
	Fcomment=	(1<<4),

	GZCRCPOLY	= 0xedb88320UL,
};

static ulong	*crctab;
static ulong	crc;

int
gunzip(uchar *out, int outn, uchar *in, int inn)
{
	Biobuf bin, bout;
	int err;

	crc = 0;
	crctab = mkcrctab(GZCRCPOLY);
	err = inflateinit();
	if(err != FlateOk)
		print("inflateinit failed: %s\n", flateerr(err));

	bin.bp = bin.p = in;
	bin.ep = in+inn;
	bout.bp = bout.p = out;
	bout.ep = out+outn;

	err = header(&bin);
	if(err != FlateOk)
		return err;

	err = inflate(&bout, crcwrite, &bin, getc);
	if(err != FlateOk)
		print("inflate failed: %s\n", flateerr(err));

	err = trailer(&bout, &bin);
	if(err != FlateOk)
		return err;

	return Boffset(&bout);
}

static int
header(Biobuf *bin)
{
	int i, flag;

	if(getc(bin) != 0x1f || getc(bin) != 0x8b){
		print("bad magic\n");
		return FlateCorrupted;
	}
	if(getc(bin) != 8){
		print("unknown compression type\n");
		return FlateCorrupted;
	}
	
	flag = getc(bin);
	
	/* mod time */
	get4(bin);
	
	/* extra flags */
	getc(bin);
	
	/* OS type */
	getc(bin);

	if(flag & Fextra)
		for(i=getc(bin); i>0; i--)
			getc(bin);
	
	/* name */
	if(flag&Fname)
		while(getc(bin) != 0)
			;

	/* comment */
	if(flag&Fcomment)
		while(getc(bin) != 0)
			;

	/* crc16 */
	if(flag&Fhcrc) {
		getc(bin);
		getc(bin);
	}
		
	return FlateOk;
}

static int
trailer(Biobuf *bout, Biobuf *bin)
{
	/* crc32 */
	ulong x;

	x = get4(bin);
	if(crc != x){
		print("crc mismatch %lux %lux\n", crc, x);
		return FlateCorrupted;
	}

	/* length */
	if(get4(bin) != Boffset(bout)){
		print("bad output len\n");
		return FlateCorrupted;
	}
	return FlateOk;
}

static ulong
get4(Biobuf *b)
{
	ulong v;
	int i, c;

	v = 0;
	for(i = 0; i < 4; i++){
		c = getc(b);
		v |= c << (i * 8);
	}
	return v;
}

static int
getc(void *in)
{
	Biobuf *bp = in;

	if((bp->p - bp->bp) % 10000 == 0)
		print(".");
	if(bp->p >= bp->ep)
		return -1;
	return *bp->p++;
}

static ulong
Boffset(Biobuf *bp)
{
	return bp->p - bp->bp;
}

static int
crcwrite(void *out, void *buf, int n)
{
	Biobuf *bp;

	crc = blockcrc(crctab, crc, buf, n);
	bp = out;
	if(n > bp->ep-bp->p)
		n = bp->ep-bp->p;
	memmove(bp->p, buf, n);
	bp->p += n;
	return n;
}

#undef malloc
#undef free

static ulong ibrkp = ~0;

void *
malloc(ulong n)
{
	ulong rv;

	if(ibrkp == ~0)
		ibrkp = ((ulong)end)+1024*1024;
	n = (n+3)>>2;
	n <<= 2;
	rv = ibrkp;
	ibrkp += n;
	return (void*)rv;
}

void
free(void *)
{
}
