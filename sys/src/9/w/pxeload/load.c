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

#include "fs.h"

Mach *m;

uint32_t *mach0pdb;
Mach *mach0m;
Segdesc *mach0gdt;
uint32_t memstart;
uint32_t memend;

int vflag = 0;
int debug = 0;

static char *diskparts[] = {"dos", "9fat", "fs", "data", "cdboot", 0};
static char *etherparts[] = {"*", 0};

static char *diskinis[] = {
    "plan9/plan9.ini",
    "plan9.ini",
    0};
static char *etherinis[] = {
    "/cfg/pxe/%E",
    0};

#define NODISCS

Type types[] = {
    {
     Tfloppy,
     Fini | Ffs,
     floppyinit, floppyinitdev,
     floppygetfspart, 0, floppyboot,
     floppyprintdevs,
     diskparts,
     diskinis,
    },
#ifndef NODISCS
    {
     Tcd,
     Fini | Ffs,
     cdinit, sdinitdev,
     sdgetfspart, sdaddconf, sdboot,
     sdprintdevs,
     diskparts,
     diskinis,
    },
#endif /* NODISCS */
    {
     Tether,
     Fini | Fbootp,
     etherinit, etherinitdev,
     pxegetfspart, 0, bootpboot,
     etherprintdevs,
     etherparts,
     etherinis,
    },
#ifndef NODISCS
    {
     Tsd,
     Fini | Ffs,
     sdinit, sdinitdev,
     sdgetfspart, sdaddconf, sdboot,
     sdprintdevs,
     diskparts,
     diskinis,
    },
#endif /* NODISCS */
    {
     Tnil,
     0,
     nil, nil, nil, nil, nil, nil,
     nil,
     nil,
     0,
     nil,
    },
};

#ifndef NODISCS

#include "sd.h"

extern SDifc sdataifc;

#ifdef NOSCSI

SDifc *sdifc[] = {
    &sdataifc,
    nil,
};

#else

extern SDifc sdmylexifc;
extern SDifc sd53c8xxifc;
SDifc *sdifc[] = {
    &sdataifc,
    &sdmylexifc,
    &sd53c8xxifc,
    nil,
};

#endif NOSCSI
#endif /* NODISCS */

typedef struct Mode Mode;

enum {
	Maxdev = 7,
	Dany = -1,
	Nmedia = 16,
	Nini = 10,
};

enum { /* mode */
       Mauto = 0x00,
       Mlocal = 0x01,
       Manual = 0x02,
       NMode = 0x03,
};

typedef struct Medium Medium;
struct Medium {
	Type *type;
	int flag;
	int dev;
	char name[NAMELEN];

	Fs *inifs;
	char *part;
	char *ini;

	Medium *next;
};

typedef struct Mode {
	char *name;
	int mode;
} Mode;

static Medium media[Nmedia];
static Medium *curmedium = media;

static Mode modes[NMode + 1] = {
    [Mauto] { "auto",   Mauto, },
    [Mlocal] { "local",  Mlocal, },
    [Manual] { "manual", Manual, },
};

char **ini;

int scsi0port;
char *defaultpartition;
int iniread;

static Medium *
parse(char *line, char **file)
{
	char *p;
	Type *tp;
	Medium *mp;

	if(p = strchr(line, '!')) {
		*p++ = 0;
		*file = p;
	} else
		*file = "";

	for(tp = types; tp->type != Tnil; tp++)
		for(mp = tp->media; mp; mp = mp->next)
			if(strcmp(mp->name, line) == 0)
				return mp;
	if(p)
		*--p = '!';
	return nil;
}

static int
boot(Medium *mp, char *file)
{
	Type *tp;
	Medium *xmp;
	static int didaddconf;
	Boot b;
	char *p;

	memset(&b, 0, sizeof b);
	b.state = INITKERNEL;

	if(didaddconf == 0) {
		didaddconf = 1;
		for(tp = types; tp->type != Tnil; tp++)
			if(tp->addconf)
				for(xmp = tp->media; xmp; xmp = xmp->next)
					(*tp->addconf)(xmp->dev);
	}

	sprint(BOOTLINE, "%s!%s", mp->name, file);
	for(p = file; *p != '\0'; p++) {
		if(*p != ' ' && *p != '\t')
			continue;
		*p = '\0';
		break;
	}
	return (*mp->type->boot)(mp->dev, file, &b);
}

static Medium *
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

Medium *
probe(int type, int flag, int dev)
{
	Type *tp;
	int i;
	Medium *mp;
	File f;
	Fs *fs;
	char **partp;

	for(tp = types; tp->type != Tnil; tp++) {
		if(type != Tany && type != tp->type)
			continue;

		if(flag != Fnone) {
			for(mp = tp->media; mp; mp = mp->next) {
				if((flag & mp->flag) && (dev == Dany || dev == mp->dev))
					return mp;
			}
		}

		if((tp->flag & Fprobe) == 0) {
			tp->flag |= Fprobe;
			tp->mask = (*tp->init)();
		}

		for(i = 0; tp->mask; i++) {
			if((tp->mask & (1 << i)) == 0)
				continue;
			tp->mask &= ~(1 << i);

			if((mp = allocm(tp)) == 0)
				continue;

			mp->dev = i;
			mp->flag = tp->flag;
			mp->type = tp;
			(*tp->initdev)(i, mp->name);

			if(mp->flag & Fini) {
				mp->flag &= ~Fini;
				for(partp = tp->parts; *partp; partp++) {
					if((fs = (*tp->getfspart)(i, *partp, 0)) == nil)
						continue;

					for(ini = tp->inis; *ini; ini++) {
						if(fswalk(fs, *ini, &f) > 0) {
							mp->inifs = fs;
							mp->part = *partp;
							mp->ini = f.path;
							mp->flag |= Fini;
							goto Break2;
						}
					}
				}
			}
		Break2:
			if((flag & mp->flag) && (dev == Dany || dev == i))
				return mp;
		}
	}

	return 0;
}

void
main(void)
{
	Medium *mp;
	int cons, flag, i, mode, tried;
	char def[2 * NAMELEN], line[80], *p, *file;
	Type *tp;

	i8042a20();
	cgapost(0x9B);

	memset(m, 0, sizeof(Mach));
	m->pdb = mach0pdb;
	m->gdt = mach0gdt;

	/*
	 * Memstart is set by the low-level bootstrap
	 * code to the first free memory location after
	 * text+data+bss+conf+pdb+pt+pmach+vmach+gdt.
	 * This is where allocatable memory starts.
	 * However, the E820 map indicates 0x000A0000
	 * is UPA memory although the VGA memory is
	 * probably there and meminit will get that wrong.
	 * Hacks are needed to make this work until it
	 * is all rewritten.
	 * Memstart->memend could be memstart->KZERO|0x9xxxx
	 * to keep all the memory use below 1MiB.
	 *
	 * Sigh. Accomodate the bloated Plan 9 kernel to prevent
	 * having different PXE loaders for different systems.
	memstart = KZERO|(3*MiB+512*KiB);
	memend = KZERO|(4*MiB);
	 */
	memstart = KZERO | (8 * MiB);
	memend = KZERO | (16 * MiB);

	cgainit();
	trapinit();
	clockinit();
	alarminit();
	spllo();
	kbdinit();
	cons = consinit("0", 0);
	readlsconf();
	meminit(0);

	if(end > KADDR(640 * 1024))
		panic("i'm too big\n");

	for(tp = types; tp->type != Tnil; tp++) {
		//if(tp->type == Tether)
		//	continue;
		if((mp = probe(tp->type, Fini, Dany)) && (mp->flag & Fini)) {
			print("using %s!%s!%s\n", mp->name, mp->part, mp->ini);
			iniread = !dotini(mp->inifs);
			break;
		}
	}
	apminit();

	if(iniread && !cons && (p = getconf("console")) != nil)
		cons = consinit(p, getconf("baud"));
	USED(cons);
	//devpccardlink();
	//devi82365link();
	if(vflag) {
		pcihinv(nil);
		delay(5000);
	}

	/*
 	 * Even after we find the ini file, we keep probing disks,
	 * because we have to collect the partition tables and
	 * have boot devices for parse.
	 */
	probe(Tany, Fnone, Dany);
	tried = 0;
	mode = Mauto;

	p = getconf("bootfile");

	if(p != 0) {
		strecpy(line, line + sizeof(line), p);
		p = line;

		mode = Manual;
		for(i = 0; i < NMode; i++) {
			if(strcmp(p, modes[i].name) == 0) {
				mode = modes[i].mode;
				goto done;
			}
		}
		if((mp = parse(p, &file)) == nil) {
			print("Unknown boot device: %s\n", p);
			goto done;
		}
		tried = boot(mp, file);
	}
done:
	if(tried == 0 && mode != Manual) {
		flag = Fany;
		if(mode == Mlocal)
			flag &= ~Fbootp;
		if((mp = probe(Tany, flag, Dany)) && mp->type->type != Tfloppy)
			boot(mp, "");
	}

	def[0] = 0;
	probe(Tany, Fnone, Dany);
	if(p = getconf("bootdef"))
		strecpy(def, def + sizeof(def), p);

	flag = 0;
	for(tp = types; tp->type != Tnil; tp++) {
		for(mp = tp->media; mp; mp = mp->next) {
			if(flag == 0) {
				flag = 1;
				print("Boot devices:");
			}
			(*tp->printdevs)(mp->dev);
		}
	}
	if(flag)
		print("\n");

	for(;;) {
		if(getstr("boot from", line, sizeof(line), def, (mode != Manual) * 15) >= 0)
			if(mp = parse(line, &file))
				boot(mp, file);
		def[0] = 0;
	}
}

int
getfields(char *lp, char **fields, int n, char sep)
{
	int i;

	for(i = 0; lp && *lp && i < n; i++) {
		while(*lp == sep)
			*lp++ = 0;
		if(*lp == 0)
			break;
		fields[i] = lp;
		while(*lp && *lp != sep) {
			if(*lp == '\\' && *(lp + 1) == '\n')
				*lp++ = ' ';
			lp++;
		}
	}
	return i;
}

int
cistrcmp(char *a, char *b)
{
	int ac, bc;

	for(;;) {
		ac = *a++;
		bc = *b++;

		if(ac >= 'A' && ac <= 'Z')
			ac = 'a' + (ac - 'A');
		if(bc >= 'A' && bc <= 'Z')
			bc = 'a' + (bc - 'A');
		ac -= bc;
		if(ac)
			return ac;
		if(bc == 0)
			break;
	}
	return 0;
}

int
cistrncmp(char *a, char *b, int n)
{
	unsigned ac, bc;

	while(n > 0) {
		ac = *a++;
		bc = *b++;
		n--;

		if(ac >= 'A' && ac <= 'Z')
			ac = 'a' + (ac - 'A');
		if(bc >= 'A' && bc <= 'Z')
			bc = 'a' + (bc - 'A');

		ac -= bc;
		if(ac)
			return ac;
		if(bc == 0)
			break;
	}

	return 0;
}

uint32_t palloc = 0;

void *
ialloc(uint32_t n, int align)
{
	uint32_t p;
	int a;

	if(palloc == 0)
		palloc = memstart;

	p = palloc;
	if(align <= 0)
		align = 4;
	if(a = n % align)
		n += align - a;
	if(a = p % align)
		p += align - a;

	palloc = p + n;
	if(palloc > memend)
		panic("ialloc(%lud, %d) called from 0x%lux\n",
		      n, align, getcallerpc(&n));
	return memset((void *)p, 0, n);
}

void *
xspanalloc(uint32_t size, int align, uint32_t span)
{
	uint32_t a, v;

	if((palloc + (size + align + span)) > memend)
		panic("xspanalloc(%lud, %d, 0x%lux) called from 0x%lux\n",
		      size, align, span, getcallerpc(&size));

	a = (uint32_t)ialloc(size + align + span, 0);

	if(span > 2)
		v = (a + span) & ~(span - 1);
	else
		v = a;

	if(align > 1)
		v = (v + align) & ~(align - 1);

	return (void *)v;
}

static Block *allocbp;

Block *
allocb(int size)
{
	Block *bp, **lbp;
	uint32_t addr;

	lbp = &allocbp;
	for(bp = *lbp; bp; bp = bp->next) {
		if((bp->lim - bp->base) >= size) {
			*lbp = bp->next;
			break;
		}
		lbp = &bp->next;
	}
	if(bp == 0) {
		if((palloc + (sizeof(Block) + size + 64)) > memend)
			panic("allocb(%d) called from 0x%lux\n",
			      size, getcallerpc(&size));
		bp = ialloc(sizeof(Block) + size + 64, 0);
		addr = (uint32_t)bp;
		addr = ROUNDUP(addr + sizeof(Block), 8);
		bp->base = (uint8_t *)addr;
		bp->lim = ((uint8_t *)bp) + sizeof(Block) + size + 64;
	}

	if(bp->flag)
		panic("allocb reuse\n");

	bp->rp = bp->base;
	bp->wp = bp->rp;
	bp->next = 0;
	bp->flag = 1;

	return bp;
}

void
freeb(Block *bp)
{
	bp->next = allocbp;
	allocbp = bp;

	bp->flag = 0;
}

enum {
	Paddr = 0x70, /* address port */
	Pdata = 0x71, /* data port */
};

uint8_t
nvramread(int offset)
{
	outb(Paddr, offset);
	return inb(Pdata);
}

void (*etherdetach)(void);
void (*floppydetach)(void);
void (*sddetach)(void);

void
impulse(void)
{
	if(etherdetach)
		etherdetach();
	if(floppydetach)
		floppydetach();
	if(sddetach)
		sddetach();

	consdrain();

	trapdisable();
}

void
warp9(uint32_t entry)
{
	extern void _warp9(uint32_t);

	print("warp9(%#lux) %d\n", entry, nmmap);
	if(vflag)
		print("mkmultiboot\n");
	mkmultiboot();
	if(vflag)
		print("impulse\n");

	/*
	 * No output after impulse().
	 */
	impulse();
	_warp9(PADDR(entry));

	/*
	 * Instead, this is where to push things on the stack to
	 * boot *BSD systems, e.g.
	(*(void(*)(void*, void*, void*, void*, ulong, ulong))(PADDR(entry)))(0, 0, 0, 0, 8196, 640);
	 * will enable NetBSD boot (the real memory size needs to
	 * go in the 5th argument).
	(*(void(*)(void))(PADDR(entry)))();
	 */
}
