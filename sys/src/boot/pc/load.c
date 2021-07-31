#include "u.h"
#include "lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#include "sd.h"
#include "fs.h"

/*
 * "cache" must be in this list so that 9load will pass the definition of
 * the cache partition into the kernel so that the disk named by the `cfs'
 * variable in plan9.ini can be seen in all circumstances before termrc
 * sets up all the disk partitions.  In particular, if it's on an odd-ball
 * disk like sd10 rather than sdC0, this is needed.
 * "nvram" is for the benefit of AoE clients.
 */
static char *diskparts[] = {
	"dos", "plan9", "9fat", "fs", "data", "cdboot", "cache", "nvram", 0
};
static char *etherparts[] = { "*", 0 };

static char *diskinis[] = {
	"plan9/plan9.ini",
	"plan9.ini",
	0
};
static char *etherinis[] = {
	"/cfg/pxe/%E",
	0
};

/* ordering: devbios must be called before devsd calls sdbios */
Type types[] = {
	{	Tfloppy,
		Fini|Ffs,
		floppyinit, floppyinitdev,
		floppygetfspart, 0, floppyboot,
		floppyprintdevs,
		diskparts,
		diskinis,
	},
	{	Tether,
		Fini|Fbootp,
		etherinit, etherinitdev,
		pxegetfspart, 0, bootpboot,
		etherprintdevs,
		etherparts,
		etherinis,
	},
	{	Tbios,
		Fini|Ffs,
		biosinit, biosinitdev,
		biosgetfspart, nil, biosboot,
		biosprintdevs,
		diskparts,
		diskinis,
	},
	{	Tcd,
		Fini|Ffs,
		cdinit, sdinitdev,
		sdgetfspart, sdaddconf, sdboot,
		sdprintdevs,
		diskparts,
		diskinis,
	},
	{	Tsd,
		Fini|Ffs,
		sdinit, sdinitdev,
		sdgetfspart, sdaddconf, sdboot,
		sdprintdevs,
		diskparts,
		diskinis,
	},
	{	Tnil,
		0,
		nil, nil, nil, nil, nil, nil,
		nil,
		nil,
		0,
		nil,
	},
};

static char *typenm[] = {
	[Tnil]		"nil",
	[Tfloppy]	"floppy",
	[Tsd]		"sd",
	[Tether]	"ether",
	[Tcd]		"cd",
	[Tbios]		"bios",
};

extern SDifc sdataifc;
extern SDifc sdiahciifc;
extern SDifc sdaoeifc;
extern SDifc sdbiosifc;

extern SDifc sdmylexifc;
extern SDifc sd53c8xxifc;

SDifc* sdifc[] = {
	&sdataifc,
	&sdiahciifc,
	&sdmylexifc,
	&sd53c8xxifc,
	&sdbiosifc,
	&sdaoeifc,
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

	Fs *inifs;
	char *part;
	char *ini;

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

char **ini;

int scsi0port;
char *defaultpartition;

/*
 * Is it safe to probe the bios?
 *
 * Empirically, we can't touch any disks natively (not through the
 * bios) if we want to read them through the bios.  So we have to
 * choose bios (usb) loading or everything-else loading.
 */
int biosload;
int askbiosload;

int iniread;
int debugload;
int vga;
char *persist;

static char *
typename(int type)
{
	if (type < 0 || type >= nelem(typenm) || typenm[type] == nil)
		return "**gok**";
	return typenm[type];
}

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
//	print("booting %s!%s\n", mp->name, file);
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

Medium*
probe(int type, int flag, int dev)
{
	Type *tp;
	int i;
	Medium *mp;
	File f;
	Fs *fs;
	char **partp;

	for(tp = types; tp->type != Tnil; tp++){
		if(type != Tany && type != tp->type)
			continue;

		if(flag != Fnone){
			for(mp = tp->media; mp; mp = mp->next){
				if((flag & mp->flag) && (dev == Dany || dev == mp->dev))
					return mp;
			}
		}
		if (debugload)
			print("probing %s...", typename(tp->type));
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

			if(mp->flag & Fini){
				mp->flag &= ~Fini;
				for(partp = tp->parts; *partp; partp++){
					if((fs = (*tp->getfspart)(i, *partp, 0)) == nil)
						continue;

					for(ini = tp->inis; *ini; ini++){
						if(fswalk(fs, *ini, &f) > 0){
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
	int flag, i, mode, tried;
	char def[2*NAMELEN], line[80], *p, *file;
	Type *tp;

	i8042a20();
	memset(m, 0, sizeof(Mach));
	trapinit();
	clockinit();
	alarminit();
	meminit(0);
	spllo();

	/*
	 * the soekris machines have no video but each has a serial port.
	 * they must see serial output, if any, before cga output because
	 * otherwise the soekris bios will translate cga output to serial
	 * output, which will garble serial console output.
	 */
	pcimatch(nil, 0, 0);		/* force scan of pci table */
	if (!vga) {
		consinit("0", "9600");	/* e.g., for soekris debugging */
		print("no vga; serial console only\n");
	}
	kbdinit();
	if((ulong)&end > (KZERO|(640*1024)))
		panic("i'm too big");

	if (!pxe)
		/* TODO turning off debug and debugload makes loading fail */
		debug = 1;

	/* this is too early for the non-soekris serial console, alas */
	if (askbiosload) {
		line[0] = '\0';
		getstr("use bios drivers (e.g., for usb)", line, sizeof line,
			"no", 60);
		biosload = strncmp(line, "yes", 3) == 0;
	}

	/*
	 * find and read plan9.ini, setting configuration variables.
	 */
	if (debug)
		print("plan9.ini probe...");
	for(tp = types; tp->type != Tnil; tp++){
		/*
		 * we don't know which ether interface to use nor
		 * whether bios loading is disabled until we have read
		 * plan9.ini.  make an exception for 9pxeload: probe
		 * ethers anyway.
		 */
		if(!pxe && tp->type == Tether /*|| !vga && tp->type == Tbios */)
			continue;
		if (debug)
			print("probing %s...", typename(tp->type));
		if((mp = probe(tp->type, Fini, Dany)) && (mp->flag & Fini)){
			if (debug)
				print("using %s!%s!%s\n",
					mp->name, mp->part, mp->ini);
			iniread = !dotini(mp->inifs);
			break;
		}
	}
	if (debug)
		print("\n");

	/*
	 * we should now have read plan9.ini, if any.
	 */
	if (!iniread)
		print("no plan9.ini\n");

	persist = getconf("*bootppersist");
	if (!pxe)
		debug = 0;		/* stop the flood of output */
	debugload = getconf("*debugload") != nil;
	/*
	 * !vga is a hack for soekris-like machines.
	 * 9pxeload can't use bios int 13 calls; they wedge the machine.
	 */
	if(!vga || pxe || getconf("*nobiosload") != nil)
		biosload = 0;
	if((p = getconf("console")) != nil)
		consinit(p, getconf("baud"));

	prcpuid();
	readlsconf();
	apminit();
	print("bios (usb) loading %s\n", biosload? "enabled": "disabled");

	devpccardlink();
	devi82365link();

	/*
 	 * Even after we find the ini file, we keep probing disks,
	 * because we have to collect the partition tables and
	 * have boot devices for parse.
	 */
	probe(Tany, Fnone, Dany);
	if (debugload)
		print("end disk probe\n");
	tried = 0;
	mode = Mauto;

	p = getconf("bootfile");
	if(p != 0) {
		mode = Manual;
		for(i = 0; i < NMode; i++){
			if(strcmp(p, modes[i].name) == 0){
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
	if(tried == 0 && mode != Manual){
		flag = Fany;
		if(mode == Mlocal)
			flag &= ~Fbootp;
		if((mp = probe(Tany, flag, Dany)) && mp->type->type != Tfloppy)
			boot(mp, "");
		if (debugload)
			print("end auto-boot probe\n");
	}

	def[0] = 0;
	probe(Tany, Fnone, Dany);
	if (debugload)
		print("end final probe\n");

	if(p = getconf("bootdef"))
		strncpy(def, p, sizeof def);

	/* print possible boot methods */
	flag = 0;
	for(tp = types; tp->type != Tnil; tp++){
		for(mp = tp->media; mp; mp = mp->next){
			if(flag == 0){
				flag = 1;
				print("Boot devices:");
			}
			(*tp->printdevs)(mp->dev);
		}
	}
	if(flag)
		print("\n");

	/*
	 * e.g., *bootppersist=ether0
	 *
	 * previously, we looped in bootpopen if we were pxeload or if
	 * *bootppersist was set.  that doesn't work well for pxeload
	 * in configurations where bootp will never succeed on the first
	 * interface but only on another interface.
	 */
	if (mode == Mauto && persist != nil &&
	    (mp = parse(persist, &file)) != nil) {
		boot(mp, file);
		print("pausing before retry...");
		delay(30*1000);
		print("\n");
	}

	for(;;){
		if(getstr("boot from", line, sizeof(line), def, (mode != Manual)*15) >= 0)
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

#define PSTART		( 8*1024*1024)
#define PEND		(16*1024*1024)

ulong palloc = PSTART;

void*
ialloc(ulong n, int align)
{
	ulong p;
	int a;

	p = palloc;
	if(align <= 0)
		align = 4;
	if(a = n % align)
		n += align - a;
	if(a = p % align)
		p += align - a;


	palloc = p+n;
	if(palloc > PEND)
		panic("ialloc(%lud, %d) called from %#p",
			n, align, getcallerpc(&n));
	return memset((void*)(p|KZERO), 0, n);
}

void*
xspanalloc(ulong size, int align, ulong span)
{
	ulong a, v;

	if((palloc + (size+align+span)) > PEND)
		panic("xspanalloc(%lud, %d, 0x%lux) called from %#p",
			size, align, span, getcallerpc(&size));

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
		if((palloc + (sizeof(Block)+size+64)) > PEND)
			panic("allocb(%d) called from %#p",
				size, getcallerpc(&size));
		bp = ialloc(sizeof(Block)+size+64, 0);
		addr = (ulong)bp;
		addr = ROUNDUP(addr + sizeof(Block), 8);
		bp->base = (uchar*)addr;
		bp->lim = ((uchar*)bp) + sizeof(Block)+size+64;
	}

	if(bp->flag)
		panic("allocb reuse");

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
	if(floppydetach)
		floppydetach();
	if(sddetach)
		sddetach();
	consdrain();

	splhi();
	trapdisable();

	/*
	 * This is where to push things on the stack to
	 * boot *BSD systems, e.g.
	(*(void(*)(void*, void*, void*, void*, ulong, ulong))(PADDR(entry)))(0, 0, 0, 0, 8196, 640);
	 * will enable NetBSD boot (the real memory size needs to
	 * go in the 5th argument).
	 */
	(*(void(*)(void))(PADDR(entry)))();
}
