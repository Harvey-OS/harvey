#include "u.h"
#include "lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"

#include "fs.h"

Type types[] = {
	{	Tfloppy,
		Fini|Ffs,
		floppyinit, floppyinitdev,
		floppygetfspart, 0, floppyboot,
	},
	{	Tsd,
		Fini|Ffs,
		sdinit, sdinitdev,
		sdgetfspart, sdaddconf, sdboot,
	},
	{	Tnil,
		0,
		0, 0,
		0, 0, 0,
	},
};

#include "sd.h"

extern SDifc sdataifc;
extern SDifc sdmylexifc;
extern SDifc sd53c8xxifc;
SDifc* sdifc[] = {
	&sdataifc,
//	&sdmylexifc,
//	&sd53c8xxifc,
	nil,
};

typedef struct Mode Mode;

enum {
	Maxdev		= 7,
	Dany		= -1,
	Nmedia		= 16,
	Nini		= 10,
};

enum {					/* mode */
	Mauto		= 0x00,
	Mlocal		= 0x01,
	Manual		= 0x02,
	NMode		= 0x03,
};

typedef struct Medium Medium;
struct Medium {
	Type*	type;
	int	flag;
	int	dev;
	char name[NAMELEN];
	Fs*	inifs;

	Medium*	next;
};

typedef struct Mode {
	char*	name;
	int	mode;
} Mode;

static Medium media[Nmedia];
static Medium *curmedium = media;

static Mode modes[NMode+1] = {
	[Mauto]		{ "auto",   Mauto,  },
	[Mlocal]	{ "local",  Mlocal, },
	[Manual]	{ "manual", Manual, },
};

char *defaultpartition = "new";

static Medium*
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
	return nil;
}

static int
boot(Medium *mp, char *file)
{
	static Boot b;

	memset(&b, 0, sizeof b);
	b.state = INIT9LOAD;

//	sprint(BOOTLINE, "%s!%s", mp->name, file);
	return (*mp->type->boot)(mp->dev, file, &b);
}

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

char *parts[] = { "dos", "9fat", "fs", 0 };

Medium*
probe(int type, int flag, int dev)
{
	Type *tp;
	int i;
	Medium *mp;

	for(tp = types; tp->type != Tnil; tp++){
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
			if((tp->mask & (1<<i)) == 0)
				continue;
			tp->mask &= ~(1<<i);

			if((mp = allocm(tp)) == 0)
				continue;

			mp->dev = i;
			mp->flag = tp->flag;
			mp->type = tp;
			(*tp->initdev)(i, mp->name);

			if((flag & mp->flag) && (dev == Dany || dev == i))
				return mp;
		}
	}

	return 0;
}

extern int loopconst;
void
main(void)
{
	Medium *mp;
	int flag;
	char def[2*NAMELEN], line[80], *p, *file;
	Type *tp;

	i8042a20();
	memset(m, 0, sizeof(Mach));
	trapinit();
	clockinit();
	alarminit();
	spllo();

	kbdinit();
	
	if((ulong)&end > (KZERO|(640*1024)))
		panic("i'm too big");

	/*
	 * If there were any arguments, MS-DOS leaves a character
	 * count followed by the arguments in the runtime header.
	 * Step over the leading space.
	 */
	p = (char*)0x80080080;
	if(p[0]){
		p[p[0]+1] = 0;
		p += 2;
	}
	else
		p = 0;

	/*
	 * Advance command line to first option, if any
	 */
	if(p) {
		while(*p==' ' || *p=='\t')
			p++;
		if(*p == 0)
			p = nil;
	}

	/*
 	 * Probe everything, to collect device names.
	 */
	probe(Tany, Fnone, Dany);

	if(p != 0) {
		if((mp = parse(p, &file)) == nil) {
			print("bad loadfile syntax: %s\n", p);
			goto done;
		}
		boot(mp, file);
	}

done:
	flag = 0;
	for(tp = types; tp->type != Tnil; tp++){
		for(mp = tp->media; mp; mp = mp->next){
			if(flag == 0){
				flag = 1;
				print("Load devices:");
			}
			print(" %s", mp->name);
		}
	}
	if(flag)
		print("\n");

	for(;;){
		if(getstr("load from", line, sizeof(line), nil, 0) >= 0)
			if(mp = parse(line, &file))
				boot(mp, file);
		def[0] = 0;
	}
}

int
getfields(char *lp, char **fields, int n, char sep)
{
	int i;

	for(i = 0; lp && *lp && i < n; i++){
		while(*lp == sep)
			*lp++ = 0;
		if(*lp == 0)
			break;
		fields[i] = lp;
		while(*lp && *lp != sep){
			if(*lp == '\\' && *(lp+1) == '\n')
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

	for(;;){
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

	while(n > 0){
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

void*
ialloc(ulong n, int align)
{

	static ulong palloc;
	ulong p;
	int a;

	if(palloc == 0)
		palloc = 3*1024*1024;

	p = palloc;
	if(align <= 0)
		align = 4;
	if(a = n % align)
		n += align - a;
	if(a = p % align)
		p += align - a;

	palloc = p+n;

	return memset((void*)(p|KZERO), 0, n);
}

void*
xspanalloc(ulong size, int align, ulong span)
{
	ulong a, v;

	a = (ulong)ialloc(size+align+span, 0);

	if(span > 2)
		v = (a + span) & ~(span-1);
	else
		v = a;

	if(align > 1)
		v = (v + align) & ~(align-1);

	return (void*)v;
}

static Block *allocbp;

Block*
allocb(int size)
{
	Block *bp, **lbp;
	ulong addr;

	lbp = &allocbp;
	for(bp = *lbp; bp; bp = bp->next){
		if((bp->lim - bp->base) >= size){
			*lbp = bp->next;
			break;
		}
		lbp = &bp->next;
	}
	if(bp == 0){
		bp = ialloc(sizeof(Block)+size+64, 0);
		addr = (ulong)bp;
		addr = ROUNDUP(addr + sizeof(Block), 8);
		bp->base = (uchar*)addr;
		bp->lim = ((uchar*)bp) + sizeof(Block)+size+64;
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
freeb(Block* bp)
{
	bp->next = allocbp;
	allocbp = bp;

	bp->flag = 0;
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

void (*etherdetach)(void);
void (*floppydetach)(void);
void (*sddetach)(void);

void
warp9(ulong entry)
{
	if(etherdetach)
		etherdetach();
	consdrain();
	(*(void(*)(void))(PADDR(entry)))();
}

char*
getconf(char*)
{
	return nil;
}

void
addconf(char*, ...)
{
}

void
uartspecial(int, void(*)(int), int(*)(void), int)
{
}

void
uartputs(IOQ*, char*, int)
{
}

void
uartputc(int)
{}

void
uartdrain(void)
{
}
