#include "u.h"
#include "lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"

#include "/sys/src/libmach/elf.h"

enum {
	Maxkernel = 4*1024*1024,
};

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
		print("readehdr OK entry 0x%lux\n", ehdr.elfentry);

	curoff = sizeof(Ehdr);
	i = ehdr.phoff+ehdr.phentsize*ehdr.phnum - curoff;
	b->state = READPHDR;
	b->bp = (char*)malloc(i);
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
	char *paddr;

	if(debug)
		print("readedata %d\n", curphdr);

	for(; curphdr < ehdr.phnum; curphdr++){
		php = phdr+curphdr;
		if(php->type != LOAD)
			continue;
		offset = php->offset;
		paddr = (char*)PADDR(php->paddr);
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
			b->bp = (char*)malloc(offset - curoff);
			b->wp = b->bp;
			b->ep = b->wp + offset - curoff;
			if(debug)
				print("nextphdr %lud...\n", offset - curoff);
			return 1;
		}
		b->state = READEDATA;
		b->bp = paddr;
		b->wp = b->bp;
		b->ep = b->wp+php->filesz;
		print("%ud+", php->filesz);
		elftotal += php->filesz;
		if(debug)
			print("nextphdr %ud@0x%p\n", php->filesz, paddr);

		return 1;
	}

	if(curphdr != 0){
		print("=%lud\n", elftotal);
		b->state = TRYBOOT;
		entry = ehdr.elfentry & ~0xF0000000;
		PLLONG(b->exec.entry, entry);
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
		memset((char*)(PADDR(php->paddr)+php->filesz), 0, php->memsz-php->filesz);
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
		print("phdr curoff %lud vaddr 0x%lux paddr 0x%lux\n",
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

int
bootpass(Boot *b, void *vbuf, int nbuf)
{
	char *buf, *ebuf;
	Exec *ep;
	ulong entry, data, text, bss;

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
			b->bp = (char*)&b->exec;
			b->wp = b->bp;
			b->ep = b->bp+sizeof(Exec);
			break;
		case READEXEC:
			ep = &b->exec;
			if(GLLONG(ep->magic) == I_MAGIC) {
				b->state = READ9TEXT;
				b->bp = (char*)PADDR(GLLONG(ep->entry));
				b->wp = b->bp;
				b->ep = b->wp+GLLONG(ep->text);
				print("%lud", GLLONG(ep->text));
				break;
			}

			/* check for gzipped kernel */
			if(b->bp[0] == 0x1F && (uchar)b->bp[1] == 0x8B && b->bp[2] == 0x08) {
				b->state = READGZIP;
				b->bp = (char*)malloc(Maxkernel);
				b->wp = b->bp;
				b->ep = b->wp + Maxkernel;
				memmove(b->bp, &b->exec, sizeof(Exec));
				b->wp += sizeof(Exec);
				print("gz...");
				break;
			}

			/*
			 * Check for ELF.
			 */
			if(memcmp(b->bp, elfident, 4) == 0){
				b->state = READEHDR;
				b->bp = (char*)&ehdr;
				b->wp = b->bp;
				b->ep = b->wp + sizeof(Ehdr);
				memmove(b->bp, &b->exec, sizeof(Exec));
				b->wp += sizeof(Exec);
				print("elf...");
				break;
			}

			print("bad kernel format\n");
			b->state = FAILED;
			return FAIL;

		case READ9TEXT:
			ep = &b->exec;
			b->state = READ9DATA;
			b->bp = (char*)PGROUND(PADDR(GLLONG(ep->entry))+GLLONG(ep->text));
			b->wp = b->bp;
			b->ep = b->wp + GLLONG(ep->data);
			print("+%ld", GLLONG(ep->data));
			break;
	
		case READ9DATA:
			ep = &b->exec;
			bss = GLLONG(ep->bss);
			print("+%ld=%ld\n",
				bss, GLLONG(ep->text)+GLLONG(ep->data)+bss);
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
		entry = GLLONG(b->exec.entry);
		print("entry: 0x%lux\n", entry);
		warp9(PADDR(entry));
		b->state = FAILED;
		return FAIL;

	case READGZIP:
		ep = &b->exec;
		if(b->bp[0] != 0x1F || (uchar)b->bp[1] != 0x8B || b->bp[2] != 0x08)
			print("lost magic\n");

		print("%ld => ", b->wp - b->bp);
		if(gunzip((uchar*)ep, sizeof(*ep), (uchar*)b->bp, b->wp - b->bp) < sizeof(*ep)) {
			print("badly compressed kernel\n");
			return FAIL;
		}

		entry = GLLONG(ep->entry);
		text = GLLONG(ep->text);
		data = GLLONG(ep->data);
		bss = GLLONG(ep->bss);
		print("%lud+%lud+%lud=%lud\n", text, data, bss, text+data+bss);

		if(gunzip((uchar*)PADDR(entry)-sizeof(Exec), sizeof(Exec)+text+data, 
		     (uchar*)b->bp, b->wp-b->bp) < sizeof(Exec)+text+data) {
			print("error uncompressing kernel\n");
			return FAIL;
		}

		/* relocate data to start at page boundary */
		memmove((void*)PGROUND(PADDR(entry+text)), (void*)(PADDR(entry+text)), data);

		print("entry: %lux\n", entry);
		warp9(PADDR(entry));
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
