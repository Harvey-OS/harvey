#include "u.h"
#include "lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"

#include "dosfs.h"

typedef struct Type Type;
typedef struct Medium Medium;
typedef struct Mode Mode;

enum {
	Maxdev		= 7,
	Dany		= -1,
	Nmedia		= 16,
};

enum {					/* type */
	Tfloppy		= 0x00,
	Thard		= 0x01,
	Tscsi		= 0x02,
	Tether		= 0x03,
	NType		= 0x04,

	Tany		= -1,
};

enum {					/* flag and name */
	Fnone		= 0x00,

	Fdos		= 0x01,
	Ndos		= 0x00,
	Fboot		= 0x02,
	Nboot		= 0x01,
	Fbootp		= 0x04,
	Nbootp		= 0x02,
	NName		= 0x03,

	Fany		= Fbootp|Fboot|Fdos,

	Fini		= 0x10,
	Freverse	= 0x20,
	Fprobe		= 0x80,
};

enum {					/* mode */
	Mauto		= 0x00,
	Mlocal		= 0x01,
	Manual		= 0x02,
	NMode		= 0x03,
};

enum
{
	CGAWIDTH	= 160,
	CGAHEIGHT	= 24,

	NLINES		= 10,
	NWIDTH		= 60,
	WLOCX		= 5,
	WLOCY		= 5,

	MENUATTR	= 7,
	HIGHMENUATTR	= 112,
};

typedef struct Type {
	int	type;
	int	flag;
	int	(*init)(void);
	long	(*read)(int, void*, long);
	long	(*seek)(int, long);
	Partition* (*setpart)(int, char*);
	char*	name[NName];

	int	mask;
	Medium*	media;
} Type;

typedef struct Medium {
	Type*	type;
	int	flag;
	Partition* partition;
	Dos;

	Medium*	next;
} Medium;

typedef struct Mode {
	char*	name;
	int	mode;
} Mode;

static Type types[NType] = {
	{	Tfloppy,
		Fini|Fdos,
		floppyinit, floppyread, floppyseek, 0,
		{ "fd", }
	},
};

static Medium media[Nmedia];
static Medium *curmedium = media;

static Mode modes[NMode+1] = {
	[Mauto]		{ "auto",   Mauto,  },
	[Mlocal]	{ "local",  Mlocal, },
	[Manual]	{ "manual", Manual, },
};

static Medium*
allocm(Type *tp)
{
	Medium **l;

	if(curmedium >= &media[Nmedia])
		return 0;

	for(l = &tp->media; *l; l = &(*l)->next)
		;
	*l = curmedium++;
	return *l;
}

void
xclrscrn(int attr)
{
	int i;

	for(i = 0; i < CGAWIDTH*CGAHEIGHT; i += 2) {
		cga[i] = ' ';
		cga[i+1] = attr;
	}
}

void
xprint(int x, int y, int attr, char *fmt, ...)
{
	uchar *p;
	int out, i;
	char buf[CGAWIDTH];

	x *= 2;

	out = doprint(buf, buf+sizeof(buf), fmt, DOTDOT) - buf;
	p = (uchar*)(cga + x + (y*CGAWIDTH));
	for(i = 0; i < out; i++) {
		*p++ = buf[i];
		*p++ = attr;
	}
}

void
xputc(int x, int y, int attr, int c)
{
	uchar *p;

	x *= 2;
	p = (uchar*)(cga + x + (y*CGAWIDTH));
	p[0] = c;
	p[1] = attr;
}

Medium*
probe(int type, int flag, int dev)
{
	Type *tp;
	int d, i, m;
	Medium *mp;

	for(tp = types; tp < &types[NType]; tp++){
		if(type != Tany && type != tp->type)
			continue;

		if(flag != Fnone){
			for(mp = tp->media; mp; mp = mp->next){
				if((flag & mp->flag) && (dev == Dany || dev == mp->dev))
					return mp;
			}
		}

		if((tp->flag & Fprobe) == 0){
			tp->flag |= Fprobe;
			tp->mask = (*tp->init)();
		}

		for(i = 0; tp->mask; i++){
			if(tp->flag & Freverse){
				m = 1<<(Maxdev-i);
				d = (Maxdev-i);
			}
			else{
				m = 1<<i;
				d = i;
			}
			if((tp->mask & m) == 0)
				continue;
			tp->mask &= ~m;

			if((mp = allocm(tp)) == 0)
				continue;

			mp->dev = d;
			mp->flag = tp->flag;
			mp->seek = tp->seek;
			mp->read = tp->read;
			mp->type = tp;

			if(mp->flag & Fboot){
				if((mp->partition = (*tp->setpart)(d, "boot")) == 0)
					mp->flag &= ~Fboot;
				(*tp->setpart)(d, "disk");
			}

			if((mp->flag & Fdos) && (dosinit(mp) < 0))
				mp->flag &= ~(Fini|Fdos);

			if((flag & mp->flag) && (dev == Dany || dev == d))
				return mp;
		}
	}

	return 0;
}

char n1[] = "System Boot Program";
char n2[] = "  Plan 9 (TM) Copyright (c) 1995 AT&T Bell Laboratories";

void
main(void)
{
	Medium *mp;
	int port;
	char scsi[64], line[80], *p;

	i8042a20();
	memset(m, 0, sizeof(Mach));
	trapinit();
	clockinit();
	alarminit();
	spllo();

	consinit();
	if((ulong)&end > (KZERO|(640*1024)))
		panic("install.com too big\n");

	xclrscrn(17);
	xprint(0, 0, 116, "%-80s", n1);
	xprint(0, 23, 116, "%-80s", n2);

	/*
	 *  look for a floppy with DOS (should be us) and boot 9dos
	 */
	mp = probe(Tfloppy, Fdos, Dany);
	if(mp == 0){
		print("Can't find the installation floppy\n");
		delay(15000);
		panic("Rebooting\n");
	}

	/*
	 *  get configuration info
	 */
	*BOOTARGS = 0;

	print("\n\nBooting the Plan 9 kernel, wait...\n");

	sprint(BOOTLINE, "%s!%d!9dos", mp->type->name[Ndos], mp->dev);
	dosboot(mp, "9dos");
	print("Can't boot 9dos\n");
	delay(15000);
	panic("Rebooting");
}

int
getfields(char *lp, char **fields, int n, char sep)
{
	int i;

	for(i=0; lp && *lp && i<n; i++){
		while(*lp == sep)
			*lp++=0;
		if(*lp == 0)
			break;
		fields[i]=lp;
		while(*lp && *lp != sep)
			lp++;
	}
	return i;
}

void*
ialloc(ulong n, int align)
{

	static struct {
		ulong	addr;
	} palloc;
	ulong p;

	if(palloc.addr == 0)
		palloc.addr = ((ulong)&end)&~KZERO;
	if(align)
		palloc.addr = PGROUND(palloc.addr);
	if((palloc.addr <= 640*1024) && (palloc.addr + n > 640*1024))
		palloc.addr = 2*1024*1024;

	memset((void*)(palloc.addr|KZERO), 0, n);
	p = palloc.addr;
	palloc.addr += n;
	if(align)
		palloc.addr = PGROUND(palloc.addr);

	return (void*)(p|KZERO);
}

enum {
	Paddr=		0x70,	/* address port */
	Pdata=		0x71,	/* data port */
};

uchar
nvramread(int offset)
{
	outb(Paddr, offset);
	return inb(Pdata);
}

char*
getconf(char*)
{
	return 0;
}
