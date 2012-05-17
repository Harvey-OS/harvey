/*
 * load kernel into memory
 */
#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"ureg.h"
#include	"pool.h"
#include	"../port/netif.h"
#include	"../ip/ip.h"
#include	"pxe.h"

#include	<a.out.h>
#include 	"/sys/src/libmach/elf.h"

#undef KADDR
#undef PADDR
// #define PADDR(a)	(paddr)((void *)(a))

#define KADDR(a)	((void*)((ulong)(a) | KZERO))
#define PADDR(a)	((ulong)(a) & ~KSEGM)

extern int debug;
extern void pagingoff(ulong);

static uchar elfident[7] = {
	'\177', 'E', 'L', 'F', '\1', '\1', '\1'
};
static Ehdr ehdr, rehdr;
static Phdr *phdr;
static int curphdr;
static ulong curoff;
static ulong elftotal;
static long (*swal)(long);
static ushort (*swab)(ushort);

/*
 * big-endian short
 */
ushort
beswab(ushort s)
{
	uchar *p;

	p = (uchar*)&s;
	return (p[0]<<8) | p[1];
}

/*
 * big-endian long
 */
long
beswal(long l)
{
	uchar *p;

	p = (uchar*)&l;
	return (p[0]<<24) | (p[1]<<16) | (p[2]<<8) | p[3];
}

/*
 * big-endian vlong
 */
uvlong
beswav(uvlong v)
{
	uchar *p;

	p = (uchar*)&v;
	return ((uvlong)p[0]<<56) | ((uvlong)p[1]<<48) | ((uvlong)p[2]<<40)
				  | ((uvlong)p[3]<<32) | ((uvlong)p[4]<<24)
				  | ((uvlong)p[5]<<16) | ((uvlong)p[6]<<8)
				  | (uvlong)p[7];
}

/*
 * little-endian short
 */
ushort
leswab(ushort s)
{
	uchar *p;

	p = (uchar*)&s;
	return (p[1]<<8) | p[0];
}

/*
 * little-endian long
 */
long
leswal(long l)
{
	uchar *p;

	p = (uchar*)&l;
	return (p[3]<<24) | (p[2]<<16) | (p[1]<<8) | p[0];
}

/*
 * Convert header to canonical form
 */
static void
hswal(long *lp, int n, long (*swap) (long))
{
	while (n--) {
		*lp = (*swap) (*lp);
		lp++;
	}
}

static int
readehdr(Boot *b)
{
	int i;

	/* bitswap the header according to the DATA format */
	if(ehdr.ident[CLASS] != ELFCLASS32) {
		print("bad ELF class - not 32 bit\n");
		return 0;
	}
	if(ehdr.ident[DATA] == ELFDATA2LSB) {
		swab = leswab;
		swal = leswal;
	} else if(ehdr.ident[DATA] == ELFDATA2MSB) {
		swab = beswab;
		swal = beswal;
	} else {
		print("bad ELF encoding - not big or little endian\n");
		return 0;
	}
	memmove(&rehdr, &ehdr, sizeof(Ehdr));

	ehdr.type = swab(ehdr.type);
	ehdr.machine = swab(ehdr.machine);
	ehdr.version = swal(ehdr.version);
	ehdr.elfentry = swal(ehdr.elfentry);
	ehdr.phoff = swal(ehdr.phoff);
	ehdr.shoff = swal(ehdr.shoff);
	ehdr.flags = swal(ehdr.flags);
	ehdr.ehsize = swab(ehdr.ehsize);
	ehdr.phentsize = swab(ehdr.phentsize);
	ehdr.phnum = swab(ehdr.phnum);
	ehdr.shentsize = swab(ehdr.shentsize);
	ehdr.shnum = swab(ehdr.shnum);
	ehdr.shstrndx = swab(ehdr.shstrndx);
	if(ehdr.type != EXEC || ehdr.version != CURRENT)
		return 0;
	if(ehdr.phentsize != sizeof(Phdr))
		return 0;

	if(debug)
		print("readehdr OK entry %#lux\n", ehdr.elfentry);

	curoff = sizeof(Ehdr);
	i = ehdr.phoff+ehdr.phentsize*ehdr.phnum - curoff;
	b->state = READPHDR;
	b->bp = (char*)smalloc(i);
	b->wp = b->bp;
	b->ep = b->wp + i;
	phdr = (Phdr*)(b->bp + ehdr.phoff-sizeof(Ehdr));
	if(debug)
		print("phdr...");

	return 1;
}

static int
nextphdr(Boot *b)
{
	Phdr *php;
	ulong entry, offset;
	char *physaddr;

	if(debug)
		print("readedata %d\n", curphdr);

	for(; curphdr < ehdr.phnum; curphdr++){
		php = phdr+curphdr;
		if(php->type != LOAD)
			continue;
		offset = php->offset;
		physaddr = (char*)KADDR(PADDR(php->paddr));
		if(offset < curoff){
			/*
			 * Can't (be bothered to) rewind the
			 * input, it might be from tftp. If we
			 * did then we could boot FreeBSD kernels
			 * too maybe.
			 */
			return 0;
		}
		if(php->offset > curoff){
			b->state = READEPAD;
			b->bp = (char*)smalloc(offset - curoff);
			b->wp = b->bp;
			b->ep = b->wp + offset - curoff;
			if(debug)
				print("nextphdr %lud...\n", offset - curoff);
			return 1;
		}
		b->state = READEDATA;
		b->bp = physaddr;
		b->wp = b->bp;
		b->ep = b->wp+php->filesz;
		print("%ud+", php->filesz);
		elftotal += php->filesz;
		if(debug)
			print("nextphdr %ud@%#p\n", php->filesz, physaddr);

		return 1;
	}

	if(curphdr != 0){
		print("=%lud\n", elftotal);
		b->state = TRYEBOOT;
		entry = ehdr.elfentry & ~KSEGM;
		PLLONG(b->hdr.entry, entry);
		return 1;
	}

	return 0;
}

static int
readepad(Boot *b)
{
	Phdr *php;

	php = phdr+curphdr;
	if(debug)
		print("readepad %d\n", curphdr);
	curoff = php->offset;

	return nextphdr(b);
}

static int
readedata(Boot *b)
{
	Phdr *php;

	php = phdr+curphdr;
	if(debug)
		print("readedata %d\n", curphdr);
	if(php->filesz < php->memsz){
		print("%lud",  php->memsz-php->filesz);
		elftotal += php->memsz-php->filesz;
		memset((char*)KADDR(PADDR(php->paddr)+php->filesz), 0,
			php->memsz-php->filesz);
	}
	curoff = php->offset+php->filesz;
	curphdr++;

	return nextphdr(b);
}

static int
readphdr(Boot *b)
{
	Phdr *php;

	php = phdr;
	hswal((long*)php, ehdr.phentsize*ehdr.phnum/sizeof(long), swal);
	if(debug)
		print("phdr curoff %lud vaddr %#lux paddr %#lux\n",
			curoff, php->vaddr, php->paddr);

	curoff = ehdr.phoff+ehdr.phentsize*ehdr.phnum;
	curphdr = 0;

	return nextphdr(b);
}

static int
addbytes(char **dbuf, char *edbuf, char **sbuf, char *esbuf)
{
	int n;

	n = edbuf - *dbuf;
	if(n <= 0)
		return 0;
	if(n > esbuf - *sbuf)
		n = esbuf - *sbuf;
	if(n <= 0)
		return -1;

	memmove(*dbuf, *sbuf, n);
	*sbuf += n;
	*dbuf += n;
	return edbuf - *dbuf;
}

void
impulse(void)
{
	delay(500);				/* drain uart */
	splhi();

	/* turn off buffered serial console */
	serialoq = nil;

	/* shutdown devices */
	chandevshutdown();
	arch->introff();
}

void
prstackuse(int)
{
	char *top, *base;
	ulong *p;

	base = up->kstack;
	top =  up->kstack + KSTACK - (sizeof(Sargs) + BY2WD);
	for (p = (ulong *)base; (char *)p < top && *p ==
	    (Stkpat<<24 | Stkpat<<16 | Stkpat<<8 | Stkpat); p++)
		;
	print("proc stack: used %ld bytes, %ld left (stack pattern)\n",
		top - (char *)p, (char *)p - base);
}

/*
 * this code is simplified from reboot().  It doesn't need to relocate
 * the new kernel nor deal with other processors.
 */
void
warp9(ulong entry)
{
//	prstackuse(0);			/* debugging */
	mkmultiboot();
	impulse();

	/* get out of KZERO space, turn off paging and jump to entry */
	pagingoff(PADDR(entry));
}

int
bootpass(Boot *b, void *vbuf, int nbuf)
{
	char *buf, *ebuf;
	uchar *sdata;
	Exechdr *hdr;
	ulong entry, pentry, text, data, bss, magic;
	uvlong entry64;

	if(b->state == FAILED)
		return FAIL;

	if(nbuf == 0)
		goto Endofinput;

	buf = vbuf;
	ebuf = buf+nbuf;
	while(addbytes(&b->wp, b->ep, &buf, ebuf) == 0) {
		switch(b->state) {
		case INITKERNEL:
			b->state = READEXEC;
			b->bp = (char*)&b->hdr;
			b->wp = b->bp;
			b->ep = b->bp+sizeof(Exechdr);
			break;
		case READEXEC:
			hdr = &b->hdr;
			magic = GLLONG(hdr->magic);
			if(magic == I_MAGIC || magic == S_MAGIC) {
				pentry = PADDR(GLLONG(hdr->entry));
				text = GLLONG(hdr->text);
				data = GLLONG(hdr->data);
				if (pentry < MB)
					panic("kernel entry %#p below 1 MB",
						pentry);
				if (PGROUND(pentry + text) + data >
				    MB + Kernelmax)
					panic("kernel larger than %d bytes",
						Kernelmax);
				b->state = READ9TEXT;
				b->bp = (char*)KADDR(pentry);
				b->wp = b->bp;
				b->ep = b->wp+text;

				if(magic == I_MAGIC){
					memmove(b->bp, b->hdr.uvl, sizeof(b->hdr.uvl));
					b->wp += sizeof(b->hdr.uvl);
				}

				print("%lud", text);
			} else if(memcmp(b->bp, elfident, 4) == 0){
				b->state = READEHDR;
				b->bp = (char*)&ehdr;
				b->wp = b->bp;
				b->ep = b->wp + sizeof(Ehdr);
				memmove(b->bp, &b->hdr, sizeof(Exechdr));
				b->wp += sizeof(Exechdr);
				print("elf...");
			} else if(b->bp[0] == 0x1F && (uchar)b->bp[1] == 0x8B &&
			    b->bp[2] == 0x08) {
				b->state = READGZIP;
				/* could use Unzipbuf instead of smalloc() */
				b->bp = (char*)smalloc(Kernelmax);
				b->wp = b->bp;
				b->ep = b->wp + Kernelmax;
				memmove(b->bp, &b->hdr, sizeof(Exechdr));
				b->wp += sizeof(Exechdr);
				print("gz...");
			} else {
				print("bad kernel format (magic %#lux)\n",
					magic);
				b->state = FAILED;
				return FAIL;
			}
			break;

		case READ9TEXT:
			hdr = &b->hdr;
			b->state = READ9DATA;
			b->bp = (char*)PGROUND((uintptr)
				KADDR(PADDR(GLLONG(hdr->entry))) +
				GLLONG(hdr->text));
			b->wp = b->bp;
			b->ep = b->wp + GLLONG(hdr->data);
			print("+%ld", GLLONG(hdr->data));
			break;
	
		case READ9DATA:
			hdr = &b->hdr;
			bss = GLLONG(hdr->bss);
			memset(b->ep, 0, bss);
			print("+%ld=%ld\n",
				bss, GLLONG(hdr->text)+GLLONG(hdr->data)+bss);
			b->state = TRYBOOT;
			return ENOUGH;

		case READEHDR:
			if(!readehdr(b)){
				print("readehdr failed\n");
				b->state = FAILED;
				return FAIL;
			}
			break;

		case READPHDR:
			if(!readphdr(b)){
				b->state = FAILED;
				return FAIL;
			}
			break;

		case READEPAD:
			if(!readepad(b)){
				b->state = FAILED;
				return FAIL;
			}
			break;

		case READEDATA:
			if(!readedata(b)){
				b->state = FAILED;
				return FAIL;
			}
			if(b->state == TRYBOOT)
				return ENOUGH;
			break;

		case TRYBOOT:
		case TRYEBOOT:
		case READGZIP:
			return ENOUGH;

		case READ9LOAD:
		case INIT9LOAD:
			panic("9load");

		default:
			panic("bootstate");
		}
	}
	return MORE;


Endofinput:
	/* end of input */
	switch(b->state) {
	case INITKERNEL:
	case READEXEC:
	case READ9TEXT:
	case READ9DATA:
	case READEHDR:
	case READPHDR:
	case READEPAD:
	case READEDATA:
		print("premature EOF\n");
		b->state = FAILED;
		return FAIL;

	case TRYBOOT:
		entry = GLLONG(b->hdr.entry);
		magic = GLLONG(b->hdr.magic);
		if(magic == I_MAGIC){
			print("entry: %#lux\n", entry);
			warp9(PADDR(entry));
		}
		else if(magic == S_MAGIC){
			entry64 = beswav(b->hdr.uvl[0]);
			warp64(entry64);
		}
		b->state = FAILED;
		return FAIL;

	case TRYEBOOT:
		entry = GLLONG(b->hdr.entry);
		if(ehdr.machine == I386){
			print("entry: %#lux\n", entry);
			warp9(PADDR(entry));
		}
		else if(ehdr.machine == AMD64){
			print("entry: %#lux\n", entry);
			warp64(entry);
		}	
		b->state = FAILED;
		return FAIL;

	case READGZIP:
		/* apparently the whole gzipped kernel is now at b->bp */
		hdr = &b->hdr;
		if(b->bp[0] != 0x1F || (uchar)b->bp[1] != 0x8B || b->bp[2] != 0x08)
			print("lost magic\n");

		print("%ld => ", b->wp - b->bp);
		/* fill hdr from gzipped b->bp to get various sizes */
		if(gunzip((uchar*)hdr, sizeof *hdr, (uchar*)b->bp, b->wp - b->bp)
		    < sizeof *hdr) {
			print("badly compressed kernel\n");
			return FAIL;
		}

		magic = GLLONG(hdr->magic);
		entry = GLLONG(hdr->entry);
		text = GLLONG(hdr->text);
		data = GLLONG(hdr->data);
		bss = GLLONG(hdr->bss);
		print("%lud+%lud+%lud=%lud\n", text, data, bss, text+data+bss);

		/* fill entry from gzipped b->bp */
		if(gunzip((uchar *)KADDR(PADDR(entry)) - sizeof(Exec),
		     sizeof(Exec)+text+data, 
		     (uchar*)b->bp, b->wp-b->bp) < sizeof(Exec)+text+data) {
			print("error uncompressing kernel\n");
			return FAIL;
		}
		/* relocate data to start at page boundary */
		sdata = KADDR(PADDR(entry+text));
		memmove((void*)PGROUND((uintptr)sdata), sdata, data);

		if(magic == I_MAGIC){
			print("entry: %#lux\n", entry);
			warp9(PADDR(entry));
		}
		else if(magic == S_MAGIC){
			entry64 = beswav(hdr->uvl[0]);
			warp64(entry64);
		} else
			print("bad magic %#lux\n", magic);
		b->state = FAILED;
		return FAIL;

	case INIT9LOAD:
	case READ9LOAD:
		panic("end 9load");

	default:
		panic("bootdone");
	}
	b->state = FAILED;
	return FAIL;
}
