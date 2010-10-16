#include <u.h>
#include <libc.h>
#include <bio.h>

#include "pci.h"
#include "vga.h"

int curprintindex;

static int iobfd = -1;
static int iowfd = -1;
static int iolfd = -1;
static int biosfd = -1;
static ulong biosoffset = 0;

enum {
	Nctlchar	= 256,
	Nattr		= 16,
};

static int ctlfd = -1;
static char ctlbuf[Nctlchar];
static int ctlclean;

static struct {
	char*	attr;
	char*	val;
} attr[Nattr];

static int
devopen(char* device, int mode)
{
	int fd;

	if((fd = open(device, mode)) < 0)
		error("devopen(%s, %d): %r\n", device, mode);
	return fd;
}

uchar
inportb(long port)
{
	uchar data;

	if(iobfd == -1)
		iobfd = devopen("#P/iob", ORDWR);

	if(pread(iobfd, &data, sizeof(data), port) != sizeof(data))
		error("inportb(0x%4.4lx): %r\n", port);
	return data;
}

void
outportb(long port, uchar data)
{
	if(iobfd == -1)
		iobfd = devopen("#P/iob", ORDWR);

	if(pwrite(iobfd, &data, sizeof(data), port) != sizeof(data))
		error("outportb(0x%4.4lx, 0x%2.2uX): %r\n", port, data);
}

ushort
inportw(long port)
{
	ushort data;

	if(iowfd == -1)
		iowfd = devopen("#P/iow", ORDWR);

	if(pread(iowfd, &data, sizeof(data), port) != sizeof(data))
		error("inportw(0x%4.4lx): %r\n", port);
	return data;
}

void
outportw(long port, ushort data)
{
	if(iowfd == -1)
		iowfd = devopen("#P/iow", ORDWR);

	if(pwrite(iowfd, &data, sizeof(data), port) != sizeof(data))
		error("outportw(0x%4.4lx, 0x%2.2uX): %r\n", port, data);
}

ulong
inportl(long port)
{
	ulong data;

	if(iolfd == -1)
		iolfd = devopen("#P/iol", ORDWR);

	if(pread(iolfd, &data, sizeof(data), port) != sizeof(data))
		error("inportl(0x%4.4lx): %r\n", port);
	return data;
}

void
outportl(long port, ulong data)
{
	if(iolfd == -1)
		iolfd = devopen("#P/iol", ORDWR);

	if(pwrite(iolfd, &data, sizeof(data), port) != sizeof(data))
		error("outportl(0x%4.4lx, 0x%2.2luX): %r\n", port, data);
}

static void
vgactlinit(void)
{
	int nattr;
	char *nl, *p, *vp;

	if(ctlclean)
		return;

	if(ctlfd == -1){
		ctlfd = devopen("#v/vgactl", ORDWR);
		memset(attr, 0, sizeof(attr));
	}

	seek(ctlfd, 0, 0);
	nattr = read(ctlfd, ctlbuf, Nctlchar-1);
	if(nattr < 0)
		error("vgactlr: read: %r\n");
	ctlbuf[nattr] = 0;

	nattr = 0;
	vp = ctlbuf;
	for(nl = strchr(ctlbuf, '\n'); nl; nl = strchr(nl, '\n')){

		*nl = '\0';
		if(p = strchr(vp, ' ')){
			*p++ = '\0';
			attr[nattr].attr = vp;
			if(*p == '\0')
				error("vgactlr: bad format: <%s>\n", vp);
			attr[nattr].val = p;
		}
		else
			error("vgactlr: bad format: <%s>\n", vp);

		if(++nattr >= Nattr-2)
			error("vgactlr: too many attributes: %d\n", nattr);
		attr[nattr].attr = 0;

		vp = ++nl;
	}

	ctlclean = 1;
}

char*
vgactlr(char* a, char* v)
{
	int i;

	trace("vgactlr: look for %s\n", a);
	vgactlinit();
	for(i = 0; attr[i].attr; i++){
		if(strcmp(attr[i].attr, a) == 0){
			strcpy(v, attr[i].val);
			trace("vgactlr: value %s\n", v);
			return v;
		}
	}
	trace("vgactlr: %s not found\n", a);

	return 0;
}

void
vgactlw(char* attr, char* val)
{
	int len;
	char buf[128];

	if(ctlfd == -1)
		ctlfd = devopen("#v/vgactl", ORDWR);

	seek(ctlfd, 0, 0);
	len = sprint(buf, "%s %s", attr, val);
	trace("+vgactlw %s\n", buf);
	if(write(ctlfd, buf, len) != len)
		error("vgactlw: <%s>: %r\n",  buf);
	trace("-vgactlw %s\n", buf);

	ctlclean = 0;
}

void
setpalette(int p, int r, int g, int b)
{
	vgao(PaddrW, p);
	vgao(Pdata, r);
	vgao(Pdata, g);
	vgao(Pdata, b);
}

static long
doreadbios(char* buf, long len, long offset)
{
	char file[64];

	if(biosfd == -1){
		biosfd = open("#v/vgabios", OREAD);
		biosoffset = 0;
	}
	if(biosfd == -1){
		snprint(file, sizeof file, "#p/%d/mem", getpid());
		biosfd = devopen(file, OREAD);
		biosoffset = 0x80000000;
	}
	if(biosfd == -1)
		return -1;
	seek(biosfd, biosoffset+offset, 0);
	return read(biosfd, buf, len);
}

char*
readbios(long len, long offset)
{
	static char bios[0x10000];
	static long biosoffset;
	static long bioslen;
	int n;

	if(biosoffset <= offset && offset+len <= biosoffset+bioslen)
		return bios+(offset - biosoffset);

	if(len > sizeof(bios))
		error("enormous bios len %ld at %lux\n", len, offset);

	n = doreadbios(bios, sizeof(bios), offset);
	if(n < len)
		error("short bios read %ld at %lux got %d\n", len,offset, n);

	biosoffset = offset;
	bioslen = n;
	return bios;
}

void
dumpbios(long size)
{
	uchar *buf;
	long offset;
	int i, n;
	char c;

	buf = alloc(size);
	offset = 0xC0000;
	if(doreadbios((char*)buf, size, offset) != size)
		error("short bios read in dumpbios\n");

	if(buf[0] != 0x55 || buf[1] != 0xAA){
		offset = 0xE0000;
		if(doreadbios((char*)buf, size, offset) != size)
			error("short bios read in dumpbios\n");
		if(buf[0] != 0x55 || buf[1] != 0xAA){
			free(buf);
			return;
		}
	}

	for(i = 0; i < size; i += 16){
		Bprint(&stdout, "0x%luX", offset+i);
		for(n = 0; n < 16; n++)
			Bprint(&stdout, " %2.2uX", buf[i+n]);
		Bprint(&stdout, "  ");
		for(n = 0; n < 16; n++){
			c = buf[i+n];
			if(c < 0x20 || c >= 0x7F)
				c = '.';
			Bprint(&stdout, "%c", c);
		}
		Bprint(&stdout, "\n");
	}
	free(buf);
}

void*
alloc(ulong nbytes)
{
	void *v;

	if((v = malloc(nbytes)) == 0)
		error("alloc: %lud bytes - %r\n", nbytes);

	return memset(v, 0, nbytes);
}

void
printitem(char* ctlr, char* item)
{
	int n;

	if(curprintindex){
		curprintindex = 0;
		Bprint(&stdout, "\n");
	}

	n = 0;
	if(ctlr && *ctlr)
		n = Bprint(&stdout, "%s ", ctlr);
	Bprint(&stdout, "%-*s", 20-n, item);
}

void
printreg(ulong data)
{
	int width;

	width = 3;
	if((curprintindex % 16) == 0 && curprintindex){
		Bprint(&stdout, "\n");
		curprintindex = 0;
		width = 23;
	}
	if(curprintindex == 8)
		Bprint(&stdout, " -");
	Bprint(&stdout, "%*.2luX", width, data);
	curprintindex++;
}

static char *flagname[32] = {
	[0x00]	"Fsnarf",
	[0x01]	"Foptions",
	[0x02]	"Finit",
	[0x03]	"Fload",
	[0x04]	"Fdump",

	[0x08]	"Hpclk2x8",
	[0x09]	"Upclk2x8",
	[0x0A]	"Henhanced",
	[0x0B]	"Uenhanced",
	[0x0C]	"Hpvram",
	[0x0D]	"Upvram",
	[0x0E]	"Hextsid",
	[0x0F]	"Uextsid",
	[0x10]	"Hclk2",
	[0x11]	"Uclk2",
	[0x12]	"Hlinear",
	[0x13]	"Ulinear",
	[0x14]	"Hclkdiv",
	[0x15]	"Uclkdiv",
	[0x16]	"Hsid32",
};

void
printflag(ulong flag)
{
	int i;
	char first;

	first = ' ';
	for(i = 31; i >= 0; i--){
		if((flag & (1<<i)) == 0)
			continue;
		if(flagname[i])
			Bprint(&stdout, "%c%s", first, flagname[i]);
		else
			Bprint(&stdout, "%c0x%x", first, 1<<i);
		first = '|';
	}
}
