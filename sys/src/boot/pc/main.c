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
	{	Tether,
		Fbootp,
		etherinit, 0, 0, 0,
		{ 0, 0, "e", },
	},
	{	Thard,
		Fini|Fboot|Fdos,
		hardinit, hardread, hardseek, sethardpart,
		{ "hd", "h", },
	},
	{	Tscsi,
		Fini|Fboot|Fdos,
		scsiinit, scsiread, scsiseek, setscsipart,
		{ "sd", "s", },
	},
};

static Medium media[Nmedia];
static Medium *curmedium = media;

static Mode modes[NMode+1] = {
	[Mauto]		{ "auto",   Mauto,  },
	[Mlocal]	{ "local",  Mlocal, },
	[Manual]	{ "manual", Manual, },
};

static char *inis[] = {
	"plan9/plan9.ini",
	"plan9.ini",
	0,
};
char **ini;

static int
parse(char *line, int *type, int *flag, int *dev, char *file)
{
	Type *tp;
	char buf[2*NAMELEN], *v[4], *p;
	int i;

	strcpy(buf, line);
	switch(getfields(buf, v, 4, '!')){

	case 3:
		break;

	case 2:
		v[2] = "";
		break;

	default:
		return 0;
	}

	*flag = 0;
	for(tp = types; tp < &types[NType]; tp++){
		for(i = 0; i < NName; i++){

			if(tp->name[i] == 0 || strcmp(v[0], tp->name[i]))
				continue;
			*type = tp->type;
			*flag |= 1<<i;

			if((*dev = strtoul(v[1], &p, 0)) == 0 && p == v[1])
				return 0;
		
			strcpy(file, v[2]);
		
			return 1;
		}
	}

	return 0;

}

static int
boot(Medium *mp, int flag, char *file)
{
	Dosfile df;
	char ixdos[128], *p;

	if(flag & Fbootp){
		sprint(BOOTLINE, "%s!%d", mp->type->name[Nbootp], mp->dev);
		return bootp(mp->dev, file);
	}

	if(flag & Fboot){
		if(mp->flag & Fini){
			(*mp->type->setpart)(mp->dev, "disk");
			plan9ini(mp);
		}
		if(file == 0 || *file == 0)
			file = mp->partition->name;
		(*mp->type->setpart)(mp->dev, file);
		sprint(BOOTLINE, "%s!%d!%s", mp->type->name[Nboot], mp->dev, file);
		return plan9boot(mp->dev, mp->seek, mp->read);
	}

	if(flag & Fdos){
		if(mp->type->setpart)
			(*mp->type->setpart)(mp->dev, "disk");
		if(mp->flag & Fini)
			plan9ini(mp);
		if(file == 0 || *file == 0){
			strcpy(ixdos, *ini);
			if(p = strrchr(ixdos, '/'))
				p++;
			else
				p = ixdos;
			strcpy(p, "9dos");
			if(dosstat(mp, ixdos, &df) <= 0)
				return -1;
		}
		else
			strcpy(ixdos, file);
		sprint(BOOTLINE, "%s!%d!%s", mp->type->name[Ndos], mp->dev, ixdos);
		return dosboot(mp, ixdos);
	}

	return -1;
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

Medium*
probe(int type, int flag, int dev)
{
	Type *tp;
	int d, i, m;
	Medium *mp;
	Dosfile df;

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

			if(mp->flag & Fini){
				mp->flag &= ~Fini;
				for(ini = inis; *ini; ini++){
					if(dosstat(mp, *ini, &df) <= 0)
						continue;
					mp->flag |= Fini;
					break;
				}
			}

			if((flag & mp->flag) && (dev == Dany || dev == d))
				return mp;
		}
	}

	return 0;
}

void
main(void)
{
	Medium *mp;
	int dev, flag, i, mode, tried, type;
	char def[2*NAMELEN], file[2*NAMELEN], line[80], *p;
	Type *tp;

	i8042a20();
	memset(m, 0, sizeof(Mach));
	trapinit();
	clockinit();
	alarminit();
	spllo();

	if((ulong)&end > (KZERO|(640*1024)))
		panic("b.com too big\n");

	for(tp = types; tp < &types[NType]; tp++){
		if(tp->type == Tether)
			continue;
		if((mp = probe(tp->type, Fini, Dany)) && (mp->flag & Fini)){
			plan9ini(mp);
			break;
		}
	}

	consinit();

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

	tried = 0;
	mode = Mauto;
	if(p == 0)
		p = getconf("bootfile");

	if(p != 0) {
		mode = Manual;
		for(i = 0; i < NMode; i++){
			if(strcmp(p, modes[i].name) == 0){
				mode = modes[i].mode;
				goto done;
			}
		}
		if(parse(p, &type, &flag, &dev, file) == 0) {
			print("Bad bootfile syntax: %s\n", p);
			goto done;
		}
		mp = probe(type, flag, dev);
		if(mp == 0) {
			print("Cannot access device: %s\n", p);
			goto done;
		}
		tried = boot(mp, flag, file);
	}
done:
	if(tried == 0 && mode != Manual){
		flag = Fany;
		if(mode == Mlocal)
			flag &= ~Fbootp;
		if((mp = probe(Tany, flag, Dany)) && mp->type->type != Tfloppy)
			boot(mp, flag & mp->flag, 0);
	}

	def[0] = 0;
	probe(Tany, Fnone, Dany);
	if(mode != Manual && (mp = probe(Tfloppy, Fdos, Dany)))
		sprint(def, "%s!%d!9dos", mp->type->name[Ndos], mp->dev);

	flag = 0;
	for(tp = types; tp < &types[NType]; tp++){
		for(mp = tp->media; mp; mp = mp->next){
			if(flag == 0){
				flag = 1;
				print("Boot devices:");
			}
	
			if(mp->flag & Fbootp)
				print(" %s!%d", mp->type->name[Nbootp], mp->dev);
			if(mp->flag & Fdos)
				print(" %s!%d", mp->type->name[Ndos], mp->dev);
			if(mp->flag & Fboot)
				print(" %s!%d", mp->type->name[Nboot], mp->dev);
		}
	}
	if(flag)
		print("\n");

	for(;;){
		if(getstr("boot from", line, sizeof(line), def, def[0]!=0) >= 0){
			if(parse(line, &type, &flag, &dev, file)){
				if(mp = probe(type, flag, dev))
					boot(mp, flag, file);
			}
		}
		def[0] = 0;
	}
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
