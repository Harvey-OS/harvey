#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"../port/error.h"

#include	<libg.h>
#include	<gnot.h>
#include	"devtab.h"
#include	"vga.h"

/*
 *  Driver for various VGA cards
 */

char	monitor[NAMELEN];	/* monitor name and type */
char	vgacard[NAMELEN];	/* vga card type */
struct screeninfo {
	int	maxx, maxy;	/* current bits per screen */
	int	packed;		/* 0=planar, 1=packed */
	int	interlaced;	/* != 0 if interlaced */
} screeninfo;

enum {
	Qdir=		0,
	Qvgamonitor=	1,
	Qvgasize=	2,
	Qvgatype=	3,
	Qvgaport=	4,
	Qvgamem=	5,
	Qmouse=		6,
	Nvga=		6,
};

Dirtab vgadir[]={
	"mouseconf",	{Qmouse},	0,		0666,
	"vgamonitor",	{Qvgamonitor},	0,		0666,
	"vgatype",	{Qvgatype},	0,		0666,
	"vgasize",	{Qvgasize},	0,		0666,
	"vgaport",	{Qvgaport},	0,		0666,
	"vgamem",	{Qvgamem},	0,		0666,
};

/* a routine from ../port/devcons.c */
extern	int readstr(ulong, char *, ulong, char *);

void
configsetup(void) {
}

void
configreset(void) {
	strcpy(monitor, "generic");
	strcpy(vgacard, "generic");
	screeninfo.maxx = 640;
	screeninfo.maxy = 480;
	screeninfo.packed = 0;
	screeninfo.interlaced = 0;
}

void
configinit(void)
{
}

Chan*
configattach(char *upec)
{
	return devattach('v', upec);
}

Chan*
configclone(Chan *c, Chan *nc)
{
	return devclone(c, nc);
}

int
configwalk(Chan *c, char *name)
{
	return devwalk(c, name, vgadir, Nvga, devgen);
}

void
configstat(Chan *c, char *dp)
{
	devstat(c, dp, vgadir, Nvga, devgen);
}

Chan*
configopen(Chan *c, int omode)
{
	return devopen(c, omode, vgadir, Nvga, devgen);
}

void
configcreate(Chan *c, char *name, int omode, ulong perm)
{
	USED(c, name, omode, perm);
	error(Eperm);
}

void
configclose(Chan *c)
{
	USED(c);
}

long
configread(Chan *c, void *buf, long n, ulong offset)
{
	char obuf[60];
	int port;
	uchar *cp = buf;

	switch(c->qid.path&~CHDIR){
	case Qdir:
		return devdirread(c, buf, n, vgadir, Nvga, devgen);
	case Qvgamonitor:
		return readstr(offset, buf, n, monitor);
	case Qvgatype:
		return readstr(offset, buf, n, vgacard);
	case Qvgasize:
		sprint(obuf, "%d %d",
			gscreen.r.max.x, gscreen.r.max.y);
		return readstr(offset, buf, n, obuf);
	case Qvgaport:
		if (offset + n >= 0x8000)
			error(Ebadarg);
		for (port=offset; port<offset+n; port++)
			*cp++ = inb(port);
		return n;
	case Qvgamem:
		if (offset > 0x10000)
			return 0;
		if (offset + n > 0x10000)
			n = 0x10000 - offset;
		memmove(cp, (void *)(SCREENMEM+offset), n);
		return n;
	}
	error(Eperm);
	return 0;
}

long
configwrite(Chan *c, void *buf, long n, ulong offset)
{
	char cbuf[20], *cp;
	int port, maxx, maxy;

	switch(c->qid.path&~CHDIR){
	case Qdir:
		error(Eperm);
	case Qvgamonitor:
		error(Eperm);
	case Qvgatype:
		error(Eperm);
	case Qvgasize:
		if(offset != 0)
			error(Ebadarg);
		if(n >= sizeof(cbuf))
			n = sizeof(cbuf) - 1;
		memmove(cbuf, buf, n);
		cbuf[n] = 0;
		cp = cbuf;
		maxx = strtoul(cp, &cp, 0);
		maxy = strtoul(cp, &cp, 0);
		if (maxx == 0 || maxy == 0 ||
		    maxx > 1280 || maxy > 1024)
			error(Ebadarg);
		setscreen(maxx, maxy, 1);
		return n;
	case Qvgaport:
		cp = buf;
		if (offset + n >= 0x8000)
			error(Ebadarg);
		for (port=offset; port<offset+n; port++) {
			outb(port, *cp++);
		}
		return n;
	case Qvgamem:
		if (offset + n > 0x10000)
			error(Ebadarg);
		memmove((void *)(SCREENMEM+offset), buf, n);
		return n;
	case Qmouse:
		if(n >= sizeof(cbuf))
			n = sizeof(cbuf) - 1;
		memmove(cbuf, buf, n);
		cbuf[n] = 0;
		if(strncmp("accelerated", cbuf, 11) == 0)
			mouseaccelerate(1);
		else if(strncmp("linear", cbuf, 6) == 0)
			mouseaccelerate(0);
		else if(strncmp("serial", cbuf, 6) == 0)
			mouseserial(atoi(cbuf+6));
		else if(strncmp("ps2", cbuf, 3) == 0)
			mouseps2();
		else if(strncmp("resolution", cbuf, 10) == 0)
			mouseres(atoi(cbuf+10));
		return n;
	}
	error(Eperm);
	return 0;
}

void
configremove(Chan *c)
{
	USED(c);
	error(Eperm);
}

void
configwstat(Chan *c, char *dp)
{
	USED(c, dp);
	error(Eperm);
}
