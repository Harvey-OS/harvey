#include <u.h>
#include <libc.h>
#include <stdarg.h>

#include "vga.h"

static int iobfd = -1;
static int iowfd = -1;
static int iolfd = -1;
static int biosfd = -1;

enum {
	Nctlchar	= 256,
	Nattr		= 16,
};

static int ctlfd = -1;
static char ctlbuf[Nctlchar];

static struct {
	char*	attr;
	char*	val;
} attr[Nattr];

static int
devopen(char *device, int mode)
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
		iobfd = devopen("#v/vgaiob", ORDWR);

	seek(iobfd, port, 0);
	if(read(iobfd, &data, sizeof(data)) != sizeof(data))
		error("inportb(0x%4.4x): %r\n", port);
	return data;
}

void
outportb(long port, uchar data)
{
	if(iobfd == -1)
		iobfd = devopen("#v/vgaiob", ORDWR);

	seek(iobfd, port, 0);
	if(write(iobfd, &data, sizeof(data)) != sizeof(data))
		error("outportb(0x%4.4x, 0x%2.2X): %r\n", port, data);
}

ushort
inportw(long port)
{
	ushort data;

	if(iowfd == -1)
		iowfd = devopen("#v/vgaiow", ORDWR);

	seek(iowfd, port, 0);
	if(read(iowfd, &data, sizeof(data)) != sizeof(data))
		error("inportw(0x%4.4x): %r\n", port);
	return data;
}

void
outportw(long port, ushort data)
{
	if(iowfd == -1)
		iowfd = devopen("#v/vgaiow", ORDWR);

	seek(iowfd, port, 0);
	if(write(iowfd, &data, sizeof(data)) != sizeof(data))
		error("outportw(0x%4.4x, 0x%2.2X): %r\n", port, data);
}

ulong
inportl(long port)
{
	ulong data;

	if(iolfd == -1)
		iolfd = devopen("#v/vgaiol", ORDWR);

	seek(iolfd, port, 0);
	if(read(iolfd, &data, sizeof(data)) != sizeof(data))
		error("inportl(0x%4.4x): %r\n", port);
	return data;
}

void
outportl(long port, ulong data)
{
	if(iolfd == -1)
		iolfd = devopen("#v/vgaiol", ORDWR);

	seek(iolfd, port, 0);
	if(write(iolfd, &data, sizeof(data)) != sizeof(data))
		error("outportl(0x%4.4x, 0x%2.2X): %r\n", port, data);
}

static void
vgactlinit(void)
{
	int nattr;
	char *nl, *p, *vp;

	if(ctlfd == -1){
		ctlfd = devopen("#v/vgactl", ORDWR);
		memset(attr, 0, sizeof(attr));
	}

	if(attr[0].attr)
		return;

	seek(ctlfd, 0, 0);
	if(read(ctlfd, ctlbuf, Nctlchar) < 0)
		error("vgactlr: read: %r\n");
	ctlbuf[Nctlchar-1] = 0;

	nattr = 0;
	vp = ctlbuf;
	for(nl = strchr(ctlbuf, '\n'); nl; nl = strchr(nl, '\n')){

		*nl = '\0';
		if(p = strtok(vp, ":")){
			attr[nattr].attr = p;
			if((p = strtok(0, " \t")) == 0)
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
}

char*
vgactlr(char *a, char *v)
{
	int i;

	vgactlinit();

	for(i = 0; attr[i].attr; i++){
		if(strcmp(attr[i].attr, a) == 0){
			strcpy(v, attr[i].val);
			return v;
		}
	}

	return 0;
}

void
vgactlw(char *attr, char *val)
{
	int len;
	char buf[128];

	if(ctlfd == -1)
		ctlfd = devopen("#v/vgactl", ORDWR);

	seek(ctlfd, 0, 0);
	len = sprint(buf, "%s %s", attr, val);
	verbose("+vgactlw %s\n", buf);
	if(write(ctlfd, buf, len) != len)
		error("vgactlw: <%s>: %r\n",  buf);
	verbose("-vgactlw %s\n", buf);
}

long
readbios(char *buf, long len, long offset)
{
	if(biosfd == -1)
		biosfd = devopen("#p/1/mem", OREAD);

	seek(biosfd, 0x80000000|offset, 0);
	if(read(biosfd, buf, len) != len)
		error("readbios read: %r\n");
	return len;
}

void
dumpbios(void)
{
	uchar buf[256];
	long offset;
	int i, n;
	char c;

	offset = 0xC0000;
	readbios((char*)buf, sizeof(buf), offset);
	if(buf[0] != 0x55 || buf[1] != 0xAA){
		offset = 0xE0000;
		readbios((char*)buf, sizeof(buf), offset);
		if(buf[0] != 0x55 || buf[1] != 0xAA)
			return;
	}

	for(i = 0; i < sizeof(buf); i += 16){
		print("0x%X", offset+i);
		for(n = 0; n < 16; n++)
			print(" %2.2uX", buf[i+n]);
		print("  ");
		for(n = 0; n < 16; n++){
			c = buf[i+n];
			if(c < 0x20 || c >= 0x7F)
				c = '.';
			print("%c", c);
		}
		print("\n");
	}
}

int vflag;

void
error(char *format, ...)
{
	char buf[512];
	int n;

	sequencer(0, 1);
	n = sprint(buf, "%s: ", argv0);
	doprint(buf+n, buf+sizeof(buf)-n, format, (&format+1));
	fprint(2, buf);
	if(vflag)
		print(buf+n);
	exits("error");
}

void*
alloc(ulong nbytes)
{
	void *v;

	if((v = malloc(nbytes)) == 0)
		error("can't malloc %d bytes - %r\n", nbytes);

	return memset(v, 0, nbytes);
}

static int curindex;

void
printitem(char *ctlr, char *item)
{
	int n;

	if(curindex){
		curindex = 0;
		print("\n");
	}

	n = 0;
	if(ctlr && *ctlr)
		n = print("%s ", ctlr);
	print("%-*s", 20-n, item);
}

void
printreg(ulong data)
{
	int width;

	width = 3;
	if((curindex % 16) == 0 && curindex){
		print("\n");
		curindex = 0;
		width = 23;
	}
	if(curindex == 8)
		print(" -");
	print("%*.2X", width, data);
	curindex++;
}

void
verbose(char *format, ...)
{
	char buf[512];

	if(vflag){
		if(curindex){
			curindex = 0;
			print("\n");
		}
		doprint(buf, buf+sizeof(buf), format, (&format+1));
		print(buf);
	}
}
