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

#define KADDR(a)	((void*)((ulong)(a) | KZERO))
#define PADDR(a)	((ulong)(a) & ~KSEGM)

extern int debug;

extern void pagingoff(ulong);

static uchar elfident[] = {
	'\177', 'E', 'L', 'F',
};
static Ehdr ehdr, rehdr;
static E64hdr e64hdr;
static Phdr *phdr;
static P64hdr *p64hdr;

static int curphdr;
static ulong curoff;
static ulong elftotal;

static uvlong (*swav)(uvlong);
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
 * little-endian vlong
 */
uvlong
leswav(uvlong v)
{
	uchar *p;

	p = (uchar*)&v;
	return ((uvlong)p[7]<<56) | ((uvlong)p[6]<<48) | ((uvlong)p[5]<<40)
				  | ((uvlong)p[4]<<32) | ((uvlong)p[3]<<24)
				  | ((uvlong)p[2]<<16) | ((uvlong)p[1]<<8)
				  | (uvlong)p[0];
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

static void
hswav(uvlong *lp, int n, uvlong (*swap)(uvlong))
{
	while (n--) {
		*lp = (*swap)(*lp);
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
	memmove(&rehdr, &ehdr, sizeof(Ehdr));	/* copy; never used */

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
	elftotal = 0;
	phdr = (Phdr*)(b->bp + ehdr.phoff-sizeof(Ehdr));
	if(debug)
		print("phdr...");

	return 1;
}

static int
reade64hdr(Boot *b)
{
	int i;

	/* bitswap the header according to the DATA format */
	if(e64hdr.ident[CLASS] != ELFCLASS64) {
		print("bad ELF class - not 64 bit\n");
		return 0;
	}
	if(e64hdr.ident[DATA] == ELFDATA2LSB) {
		swab = leswab;
		swal = leswal;
		swav = leswav;
	} else if(e64hdr.ident[DATA] == ELFDATA2MSB) {
		swab = beswab;
		swal = beswal;
		swav = beswav;
	} else {
		print("bad ELF encoding - not big or little endian\n");
		return 0;
	}
//	memmove(&rehdr, &ehdr, sizeof(Ehdr));	/* copy; never used */

	e64hdr.type = swab(e64hdr.type);
	e64hdr.machine = swab(e64hdr.machine);
	e64hdr.version = swal(e64hdr.version);
	e64hdr.elfentry = swav(e64hdr.elfentry);
	e64hdr.phoff = swav(e64hdr.phoff);
	e64hdr.shoff = swav(e64hdr.shoff);
	e64hdr.flags = swal(e64hdr.flags);
	e64hdr.ehsize = swab(e64hdr.ehsize);
	e64hdr.phentsize = swab(e64hdr.phentsize);
	e64hdr.phnum = swab(e64hdr.phnum);
	e64hdr.shentsize = swab(e64hdr.shentsize);
	e64hdr.shnum = swab(e64hdr.shnum);
	e64hdr.shstrndx = swab(e64hdr.shstrndx);
	if(e64hdr.type != EXEC || e64hdr.version != CURRENT)
		return 0;
	if(e64hdr.phentsize != sizeof(P64hdr))
		return 0;

	if(debug)
		print("reade64hdr OK entry %#llux\n", e64hdr.elfentry);

	curoff = sizeof(E64hdr);
	i = e64hdr.phoff + e64hdr.phentsize*e64hdr.phnum - curoff;
	b->state = READ64PHDR;
	b->bp = (char*)smalloc(i);
	b->wp = b->bp;
	b->ep = b->wp + i;
	elftotal = 0;
	p64hdr = (P64hdr*)(b->bp + e64hdr.phoff-sizeof(E64hdr));
	if(debug)
		print("p64hdr...");

	return 1;
}

static int
nextphdr(Boot *b)
{
	Phdr *php;
	ulong offset;
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
		b->entry = ehdr.elfentry;
		// PLLONG(b->hdr.entry, b->entry);
		return 1;
	}

	return 0;
}

static int
nextp64hdr(Boot *b)
{
	P64hdr *php;
	uvlong offset;
	char *physaddr;

	if(debug)
		print("reade64data %d\n", curphdr);

	for(; curphdr < e64hdr.phnum; curphdr++){
		php = p64hdr+curphdr;
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
			b->state = READE64PAD;
			b->bp = (char*)smalloc(offset - curoff);
			b->wp = b->bp;
			b->ep = b->wp + offset - curoff;
			if(debug)
				print("nextp64hdr %llud...\n", offset - curoff);
			return 1;
		}
		b->state = READE64DATA;
		b->bp = physaddr;
		b->wp = b->bp;
		b->ep = b->wp+php->filesz;
		print("%llud+", php->filesz);
		elftotal += php->filesz;
		if(debug)
			print("nextp64hdr %llud@%#p\n", php->filesz, physaddr);

		return 1;
	}

	if(curphdr != 0){
		print("=%lud\n", elftotal);
		b->state = TRYE64BOOT;
		b->entry = e64hdr.elfentry;
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
reade64pad(Boot *b)
{
	P64hdr *php;

	php = p64hdr+curphdr;
	if(debug)
		print("reade64pad %d\n", curphdr);
	curoff = php->offset;

	return nextp64hdr(b);
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
reade64data(Boot *b)
{
	P64hdr *php;

	php = p64hdr+curphdr;
	if(debug)
		print("reade64data %d\n", curphdr);
	if(php->filesz < php->memsz){
		print("%llud",  php->memsz - php->filesz);
		elftotal += php->memsz - php->filesz;
		memset((char*)KADDR(PADDR(php->paddr) + php->filesz), 0,
			php->memsz - php->filesz);
	}
	curoff = php->offset + php->filesz;
	curphdr++;

	return nextp64hdr(b);
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
readp64hdr(Boot *b)
{
	int hdr;
	P64hdr *php, *p;

	php = p = p64hdr;
	for (hdr = 0; hdr < e64hdr.phnum; hdr++, p++) {
		hswal((long*)p, 2, swal);
		hswav((uvlong*)&p->offset, 6, swav);
	}
	if(debug)
		print("p64hdr curoff %lud vaddr %#llux paddr %#llux\n",
			curoff, php->vaddr, php->paddr);

	curoff = e64hdr.phoff + e64hdr.phentsize*e64hdr.phnum;
	curphdr = 0;

	return nextp64hdr(b);
}

static int
addbytes(char **dbuf, char *edbuf, char **sbuf, char *esbuf)
{
	int n;

	n = edbuf - *dbuf;
	if(n <= 0)
		return 0;			/* dest buffer is full */
	if(n > esbuf - *sbuf)
		n = esbuf - *sbuf;
	if(n <= 0)
		return -1;			/* src buffer is empty */

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

static int
bootfail(Boot *b)
{
	b->state = FAILED;
	return FAIL;
}

static int
isgzipped(uchar *p)
{
	return p[0] == 0x1F && p[1] == 0x8B && p[2] == 0x08;
}

static int
readexec(Boot *b)
{
	Exechdr *hdr;
	ulong pentry, text, data, magic;

	hdr = &b->hdr;
	magic = GLLONG(hdr->magic);
	if(magic == I_MAGIC || magic == S_MAGIC) {
		pentry = PADDR(GLLONG(hdr->entry));
		text = GLLONG(hdr->text);
		data = GLLONG(hdr->data);
		if (pentry < MB)
			panic("kernel entry %#p below 1 MB", pentry);
		if (PGROUND(pentry + text) + data > MB + Kernelmax)
			panic("kernel larger than %d bytes", Kernelmax);
		b->state = READ9TEXT;
		b->bp = (char*)KADDR(pentry);
		b->wp = b->bp;
		b->ep = b->wp+text;

		if(magic == I_MAGIC){
			memmove(b->bp, b->hdr.uvl, sizeof(b->hdr.uvl));
			b->wp += sizeof(b->hdr.uvl);
		}

		print("%lud", text);
	} else if(memcmp(b->bp, elfident, 4) == 0 &&
	    (uchar)b->bp[4] == ELFCLASS32){
		b->state = READEHDR;
		b->bp = (char*)&ehdr;
		b->wp = b->bp;
		b->ep = b->wp + sizeof(Ehdr);
		memmove(b->bp, &b->hdr, sizeof(Exechdr));
		b->wp += sizeof(Exechdr);
		print("elf...");
	} else if(memcmp(b->bp, elfident, 4) == 0 &&
	    (uchar)b->bp[4] == ELFCLASS64){
		b->state = READE64HDR;
		b->bp = (char*)&e64hdr;
		b->wp = b->bp;
		b->ep = b->wp + sizeof(E64hdr);
		memmove(b->bp, &b->hdr, sizeof(Exechdr));
		b->wp += sizeof(Exechdr);
		print("elf64...");
	} else if(isgzipped((uchar *)b->bp)) {
		b->state = READGZIP;
		/* could use Unzipbuf instead of smalloc() */
		b->bp = (char*)smalloc(Kernelmax);
		b->wp = b->bp;
		b->ep = b->wp + Kernelmax;
		memmove(b->bp, &b->hdr, sizeof(Exechdr));
		b->wp += sizeof(Exechdr);
		print("gz...");
	} else {
		print("bad kernel format (magic %#lux)\n", magic);
		return bootfail(b);
	}
	return MORE;
}

static void
boot9(Boot *b, ulong magic, ulong entry)
{
	if(magic == I_MAGIC){
		print("entry: %#lux\n", entry);
		warp9(PADDR(entry));
	}
	else if(magic == S_MAGIC)
		warp64(beswav(b->hdr.uvl[0]));
	else
		print("bad magic %#lux\n", magic);
}

/* only returns upon failure */
static void
readgzip(Boot *b)
{
	ulong entry, text, data, bss, magic, all, pentry;
	uchar *sdata;
	Exechdr *hdr;

	/* the whole gzipped kernel is now at b->bp */
	hdr = &b->hdr;
	if(!isgzipped((uchar *)b->bp)) {
		print("lost magic\n");
		return;
	}
	print("%ld => ", b->wp - b->bp);
	/* just fill hdr from gzipped b->bp, to get various sizes */
	if(gunzip((uchar*)hdr, sizeof *hdr, (uchar*)b->bp, b->wp - b->bp)
	    < sizeof *hdr) {
		print("error uncompressing kernel exec header\n");
		return;
	}

	/* assume uncompressed kernel is a plan 9 boot image */
	magic = GLLONG(hdr->magic);
	entry = GLLONG(hdr->entry);
	text = GLLONG(hdr->text);
	data = GLLONG(hdr->data);
	bss = GLLONG(hdr->bss);
	print("%lud+%lud+%lud=%lud\n", text, data, bss, text+data+bss);

	pentry = PADDR(entry);
	if (pentry < MB)
		panic("kernel entry %#p below 1 MB", pentry);
	if (PGROUND(pentry + text) + data > MB + Kernelmax)
		panic("kernel larger than %d bytes", Kernelmax);

	/* fill entry from gzipped b->bp */
	all = sizeof(Exec) + text + data;
	if(gunzip((uchar *)KADDR(PADDR(entry)) - sizeof(Exec), all,
	    (uchar*)b->bp, b->wp - b->bp) < all) {
		print("error uncompressing kernel\n");
		return;
	}

	/* relocate data to start at page boundary */
	sdata = KADDR(PADDR(entry+text));
	memmove((void*)PGROUND((uintptr)sdata), sdata, data);

	boot9(b, magic, entry);
}

/*
 * if nbuf is zero, boot.
 * else add nbuf bytes from vbuf to b->wp (if there is room)
 * and advance the state machine, which may reset b's pointers
 * and return to the top.
 */
int
bootpass(Boot *b, void *vbuf, int nbuf)
{
	char *buf, *ebuf;
	Exechdr *hdr;
	ulong entry, bss;
	uvlong entry64;

	if(b->state == FAILED)
		return FAIL;

	if(nbuf == 0)
		goto Endofinput;

	buf = vbuf;
	ebuf = buf+nbuf;
	/* possibly copy into b->wp from buf (not first time) */
	while(addbytes(&b->wp, b->ep, &buf, ebuf) == 0) {
		/* b->bp is full, so advance the state machine */
		switch(b->state) {
		case INITKERNEL:
			b->state = READEXEC;
			b->bp = (char*)&b->hdr;
			b->wp = b->bp;
			b->ep = b->bp+sizeof(Exechdr);
			break;
		case READEXEC:
			readexec(b);
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

		/*
		 * elf
		 */
		case READEHDR:
			if(!readehdr(b))
				print("readehdr failed\n");
			break;
		case READPHDR:
			readphdr(b);
			break;
		case READEPAD:
			readepad(b);
			break;
		case READEDATA:
			readedata(b);
			if(b->state == TRYEBOOT)
				return ENOUGH;
			break;

		/*
		 * elf64
		 */
		case READE64HDR:
			if(!reade64hdr(b))
				print("reade64hdr failed\n");
			break;
		case READ64PHDR:
			readp64hdr(b);
			break;
		case READE64PAD:
			reade64pad(b);
			break;
		case READE64DATA:
			reade64data(b);
			if(b->state == TRYE64BOOT)
				return ENOUGH;
			break;

		case TRYBOOT:
		case TRYEBOOT:
		case TRYE64BOOT:
		case READGZIP:
			return ENOUGH;

		case READ9LOAD:
		case INIT9LOAD:
			panic("9load");

		default:
			panic("bootstate");
		}
		if(b->state == FAILED)
			return FAIL;
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
	case READE64HDR:
	case READ64PHDR:
	case READE64PAD:
	case READE64DATA:
		print("premature EOF\n");
		break;

	case TRYBOOT:
		boot9(b, GLLONG(b->hdr.magic), GLLONG(b->hdr.entry));
		break;

	case TRYEBOOT:
		entry = b->entry;
		if(ehdr.machine == I386){
			print("entry: %#lux\n", entry);
			warp9(PADDR(entry));
		}
		else if(ehdr.machine == AMD64)
			warp64(entry);
		else
			panic("elf boot: ehdr.machine %d unknown", ehdr.machine);
		break;

	case TRYE64BOOT:
		entry64 = b->entry;
		if(e64hdr.machine == I386){
			print("entry: %#llux\n", entry64);
			warp9(PADDR(entry64));
		}
		else if(e64hdr.machine == AMD64)
			warp64(entry64);
		else
			panic("elf64 boot: e64hdr.machine %d unknown",
				e64hdr.machine);
		break;

	case READGZIP:
		readgzip(b);
		break;

	case INIT9LOAD:
	case READ9LOAD:
		panic("end 9load");

	default:
		panic("bootdone");
	}
	return bootfail(b);
}
