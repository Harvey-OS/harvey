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

char	vgacard[NAMELEN];	/* vga card type */
struct screeninfo {
	int	maxx, maxy;	/* current bits per screen */
	int	packed;		/* 0=planar, 1=packed */
	int	interlaced;	/* != 0 if interlaced */
} screeninfo;

enum {
	Qdir=		0,
	Qvgasize=	1,
	Qvgatype=	2,
	Qvgaport=	3,
	Nvga=		4,
};

Dirtab vgadir[]={
	"vgatype",	{Qvgatype},	0,		0666,
	"vgasize",	{Qvgasize},	0,		0666,
	"vgaport",	{Qvgaport},	0,		0666,
};

/* a routine from ../port/devcons.c */
extern	int readstr(ulong, char *, ulong, char *);

void
configsetup(void) {
}

void
configreset(void) {
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
	int port;
	uchar *cp = buf;

	switch(c->qid.path&~CHDIR){
	case Qdir:
		return devdirread(c, buf, n, vgadir, Nvga, devgen);
	case Qvgatype:
		return readstr(offset, buf, n, vgacard);
	case Qvgaport:
		if (offset + n >= 0x8000)
			error(Ebadarg);
		for (port=offset; port<offset+n; port++)
			*cp++ = inb(port);
		return n;
	}
	error(Eperm);
	return 0;
}

long
configwrite(Chan *c, void *buf, long n, ulong offset)
{
	char cbuf[20], *cp;
	int port, maxx, maxy, ldepth;

	switch(c->qid.path&~CHDIR){
	case Qdir:
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
		if(*cp!=0)
			cp++;
		maxy = strtoul(cp, &cp, 0);
		if(*cp!=0)
			cp++;
		switch(strtoul(cp, &cp, 0)){
		case 1:
			ldepth = 0;
			break;
		case 2:
			ldepth = 1;
			break;
		case 4:
			ldepth = 2;
			break;
		case 8:
			ldepth = 3;
			break;
		default:
			ldepth = -1;
		}
		if(maxx == 0 || maxy == 0
		|| maxx > 1280 || maxy > 1024
		|| ldepth > 3 || ldepth < 0)
			error(Ebadarg);
		setscreen(maxx, maxy, ldepth);
		return n;
	case Qvgaport:
		cp = buf;
		if (offset + n >= 0x8000)
			error(Ebadarg);
		for (port=offset; port<offset+n; port++) {
			outb(port, *cp++);
		}
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
