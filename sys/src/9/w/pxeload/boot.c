/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "u.h"
#include "lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"

#include "/sys/src/libmach/elf.h"

static uint8_t elfident[7] = {
    '\177', 'E', 'L', 'F', '\1', '\1', '\1'};
static Ehdr ehdr, rehdr;
static Phdr *phdr;
static int curphdr;
static uint32_t curoff;
static uint32_t elftotal;
static int32_t (*swal)(int32_t);
static uint16_t (*swab)(uint16_t);

/*
 * big-endian short
 */
uint16_t
beswab(uint16_t s)
{
	uint8_t *p;

	p = (uint8_t *)&s;
	return (p[0] << 8) | p[1];
}

/*
 * big-endian long
 */
int32_t
beswal(int32_t l)
{
	uint8_t *p;

	p = (uint8_t *)&l;
	return (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
}

/*
 * big-endian vlong
 */
uint64_t
beswav(uint64_t v)
{
	uint8_t *p;

	p = (uint8_t *)&v;
	return ((uint64_t)p[0] << 56) | ((uint64_t)p[1] << 48) | ((uint64_t)p[2] << 40) | ((uint64_t)p[3] << 32) | ((uint64_t)p[4] << 24) | ((uint64_t)p[5] << 16) | ((uint64_t)p[6] << 8) | (uint64_t)p[7];
}

/*
 * little-endian short
 */
uint16_t
leswab(uint16_t s)
{
	uint8_t *p;

	p = (uint8_t *)&s;
	return (p[1] << 8) | p[0];
}

/*
 * little-endian long
 */
int32_t
leswal(int32_t l)
{
	uint8_t *p;

	p = (uint8_t *)&l;
	return (p[3] << 24) | (p[2] << 16) | (p[1] << 8) | p[0];
}

/*
 * Convert header to canonical form
 */
static void
hswal(int32_t *lp, int n, int32_t (*swap)(int32_t))
{
	while(n--) {
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
	i = ehdr.phoff + ehdr.phentsize * ehdr.phnum - curoff;
	b->state = READPHDR;
	b->bp = (char *)malloc(i);
	b->wp = b->bp;
	b->ep = b->wp + i;
	phdr = (Phdr *)(b->bp + ehdr.phoff - sizeof(Ehdr));
	if(debug)
		print("phdr...");

	return 1;
}

static int
nextphdr(Boot *b)
{
	Phdr *php;
	uint32_t entry, offset;
	char *paddr;

	if(debug)
		print("readedata %d\n", curphdr);

	for(; curphdr < ehdr.phnum; curphdr++) {
		php = phdr + curphdr;
		if(php->type != LOAD)
			continue;
		offset = php->offset;
		paddr = (char *)PADDR(php->paddr);
		if(offset < curoff) {
			/*
			 * Can't (be bothered to) rewind the
			 * input, it might be from tftp. If we
			 * did then we could boot FreeBSD kernels
			 * too maybe.
			 */
			return 0;
		}
		if(php->offset > curoff) {
			b->state = READEPAD;
			b->bp = (char *)malloc(offset - curoff);
			b->wp = b->bp;
			b->ep = b->wp + offset - curoff;
			if(debug)
				print("nextphdr %lud...\n", offset - curoff);
			return 1;
		}
		b->state = READEDATA;
		b->bp = paddr;
		b->wp = b->bp;
		b->ep = b->wp + php->filesz;
		print("%ud+", php->filesz);
		elftotal += php->filesz;
		if(debug)
			print("nextphdr %ud@0x%p\n", php->filesz, paddr);

		return 1;
	}

	if(curphdr != 0) {
		print("=%lud\n", elftotal);
		b->state = TRYEBOOT;
		entry = ehdr.elfentry & ~0xF0000000;
		PLLONG(b->hdr.entry, entry);
		return 1;
	}

	return 0;
}

static int
readepad(Boot *b)
{
	Phdr *php;

	php = phdr + curphdr;
	if(debug)
		print("readepad %d\n", curphdr);
	curoff = php->offset;

	return nextphdr(b);
}

static int
readedata(Boot *b)
{
	Phdr *php;

	php = phdr + curphdr;
	if(debug)
		print("readedata %d\n", curphdr);
	if(php->filesz < php->memsz) {
		print("%lud", php->memsz - php->filesz);
		elftotal += php->memsz - php->filesz;
		memset((char *)(PADDR(php->paddr) + php->filesz), 0,
		       php->memsz - php->filesz);
	}
	curoff = php->offset + php->filesz;
	curphdr++;

	return nextphdr(b);
}

static int
readphdr(Boot *b)
{
	Phdr *php;

	php = phdr;
	hswal((int32_t *)php, ehdr.phentsize * ehdr.phnum / sizeof(int32_t), swal);
	if(debug)
		print("phdr curoff %lud vaddr 0x%lux paddr 0x%lux\n",
		      curoff, php->vaddr, php->paddr);

	curoff = ehdr.phoff + ehdr.phentsize * ehdr.phnum;
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
	Hdr *hdr;
	uint32_t magic, entry, data, text, bss;
	uint64_t entry64;

	if(b->state == FAILED)
		return FAIL;

	if(nbuf == 0)
		goto Endofinput;

	buf = vbuf;
	ebuf = buf + nbuf;
	while(addbytes(&b->wp, b->ep, &buf, ebuf) == 0) {
		switch(b->state) {
		case INITKERNEL:
			b->state = READEXEC;
			b->bp = (char *)&b->hdr;
			b->wp = b->bp;
			b->ep = b->bp + sizeof(Hdr);
			break;
		case READEXEC:
			hdr = &b->hdr;
			magic = GLLONG(hdr->magic);
			if(magic == I_MAGIC || magic == S_MAGIC) {
				b->state = READ9TEXT;
				b->bp = (char *)PADDR(GLLONG(hdr->entry));
				b->wp = b->bp;
				b->ep = b->wp + GLLONG(hdr->text);

				if(magic == I_MAGIC) {
					memmove(b->bp, b->hdr.uvl, sizeof(b->hdr.uvl));
					b->wp += sizeof(b->hdr.uvl);
				}

				print("%lud", GLLONG(hdr->text));
				break;
			}

			/* check for gzipped kernel */
			if(b->bp[0] == 0x1F && (uint8_t)b->bp[1] == 0x8B && b->bp[2] == 0x08) {
				b->state = READGZIP;
				b->bp = (char *)malloc(1440 * 1024);
				b->wp = b->bp;
				b->ep = b->wp + 1440 * 1024;
				memmove(b->bp, &b->hdr, sizeof(Hdr));
				b->wp += sizeof(Hdr);
				print("gz...");
				break;
			}

			/*
			 * Check for ELF.
			 */
			if(memcmp(b->bp, elfident, 4) == 0) {
				b->state = READEHDR;
				b->bp = (char *)&ehdr;
				b->wp = b->bp;
				b->ep = b->wp + sizeof(Ehdr);
				memmove(b->bp, &b->hdr, sizeof(Hdr));
				b->wp += sizeof(Hdr);
				print("elf...");
				break;
			}

			print("bad kernel format (magic == %#lux)\n", magic);
			b->state = FAILED;
			return FAIL;

		case READ9TEXT:
			hdr = &b->hdr;
			b->state = READ9DATA;
			b->bp = (char *)PGROUND(PADDR(GLLONG(hdr->entry)) + GLLONG(hdr->text));
			b->wp = b->bp;
			b->ep = b->wp + GLLONG(hdr->data);
			print("+%ld", GLLONG(hdr->data));
			break;

		case READ9DATA:
			hdr = &b->hdr;
			bss = GLLONG(hdr->bss);
			memset(b->ep, 0, bss);
			print("+%ld=%ld\n",
			      bss, GLLONG(hdr->text) + GLLONG(hdr->data) + bss);
			b->state = TRYBOOT;
			return ENOUGH;

		case READEHDR:
			if(!readehdr(b)) {
				print("readehdr failed\n");
				b->state = FAILED;
				return FAIL;
			}
			break;

		case READPHDR:
			if(!readphdr(b)) {
				b->state = FAILED;
				return FAIL;
			}
			break;

		case READEPAD:
			if(!readepad(b)) {
				b->state = FAILED;
				return FAIL;
			}
			break;

		case READEDATA:
			if(!readedata(b)) {
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
		if(magic == I_MAGIC) {
			print("entry: 0x%lux\n", entry);
			warp9(PADDR(entry));
		} else if(magic == S_MAGIC) {
			entry64 = beswav(b->hdr.uvl[0]);
			warp64(entry64);
		}
		b->state = FAILED;
		return FAIL;

	case TRYEBOOT:
		entry = GLLONG(b->hdr.entry);
		if(ehdr.machine == I386) {
			print("entry: 0x%lux\n", entry);
			warp9(PADDR(entry));
		} else if(ehdr.machine == AMD64) {
			print("entry: 0x%lux\n", entry);
			warp64(entry);
		}
		b->state = FAILED;
		return FAIL;

	case READGZIP:
		hdr = &b->hdr;
		if(b->bp[0] != 0x1F || (uint8_t)b->bp[1] != 0x8B || b->bp[2] != 0x08)
			print("lost magic\n");

		print("%ld => ", b->wp - b->bp);
		if(gunzip((uint8_t *)hdr, sizeof(*hdr), (uint8_t *)b->bp, b->wp - b->bp) < sizeof(*hdr)) {
			print("badly compressed kernel\n");
			return FAIL;
		}

		entry = GLLONG(hdr->entry);
		text = GLLONG(hdr->text);
		data = GLLONG(hdr->data);
		bss = GLLONG(hdr->bss);
		print("%lud+%lud+%lud=%lud\n", text, data, bss, text + data + bss);

		if(gunzip((uint8_t *)PADDR(entry) - sizeof(Exec), sizeof(Exec) + text + data,
			  (uint8_t *)b->bp, b->wp - b->bp) < sizeof(Exec) + text + data) {
			print("error uncompressing kernel\n");
			return FAIL;
		}

		/* relocate data to start at page boundary */
		memmove((void *)PGROUND(PADDR(entry + text)), (void *)(PADDR(entry + text)), data);

		entry = GLLONG(b->hdr.entry);
		magic = GLLONG(b->hdr.magic);
		if(magic == I_MAGIC) {
			print("entry: 0x%lux\n", entry);
			warp9(PADDR(entry));
		} else if(magic == S_MAGIC) {
			entry64 = beswav(b->hdr.uvl[0]);
			warp64(entry64);
		}
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
