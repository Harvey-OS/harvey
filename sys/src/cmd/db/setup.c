
/*
 * init routines
 */
#include "defs.h"
#include "fns.h"

char	*symfil;
char	*corfil;

Map	*symmap;
Map	*cormap;
Map	*dotmap;

int fsym, fcor;
static Fhdr fhdr;

static int getfile(char*, int, int);

void
setsym(void)
{
	Symbol s;

	if((fsym = getfile(symfil, 1, wtflag)) < 0) {
		symmap = dumbmap(-1);
		return;
	}
	if (crackhdr(fsym, &fhdr)) {
		machbytype(fhdr.type);
		symmap = loadmap(symmap, fsym, &fhdr);
		if (symmap == 0)
			symmap = dumbmap(fsym);
		if (syminit(fsym, &fhdr) < 0)
			dprint("%r\n");
		if (mach->sbreg && lookup(0, mach->sbreg, &s))
			mach->sb = s.value;
	}
	else
		symmap = dumbmap(fsym);
}

void
setcor(void)
{
	int i;

	if (cormap) {
		for (i = 0; i < cormap->nsegs; i++)
			if (cormap->seg[i].inuse)
				close(cormap->seg[i].fd);
	}

	fcor = getfile(corfil, 2, ORDWR);
	if (fcor <= 0) {
		if (cormap)
			free(cormap);
		cormap = dumbmap(-1);
		return;
	}
	if(pid > 0) {	/* provide addressability to executing process */
		cormap = attachproc(pid, kflag, fcor, &fhdr);
		if (!cormap)
			cormap = dumbmap(-1);
	} else {
		cormap = newmap(cormap, 2);
		if (!cormap)
			cormap = dumbmap(-1);
		setmap(cormap, fcor, fhdr.txtaddr, fhdr.txtaddr+fhdr.txtsz, fhdr.txtaddr, "text");
		setmap(cormap, fcor, fhdr.dataddr, 0xffffffff, fhdr.dataddr, "data");
	}
	kmsys();
	return;
}

Map *
dumbmap(int fd)
{
	Map *dumb;

	extern Mach mi386;
	extern Machdata i386mach;

	dumb = newmap(0, 1);
	setmap(dumb, fd, 0, 0xffffffff, 0, "data");
	if (!mach) 			/* default machine = 386 */
		mach = &mi386;
	if (!machdata)
		machdata = &i386mach;
	return dumb;
}

/*
 * set up maps for a direct process image (/proc)
 */

void
cmdmap(Map *map)
{
	int i;
	char name[MAXSYM];

	extern char lastc;

	rdc();
	readsym(name);
	i = findseg(map, name);
	if (i < 0)	/* not found */
		error("Invalid map name");

	if (expr(0)) {
		if (strcmp(name, "text") == 0)
			textseg(expv, &fhdr);
		map->seg[i].b = expv;
	} else
		error("Invalid base address"); 
	if (expr(0))
		map->seg[i].e = expv;
	else
		error("Invalid end address"); 
	if (expr(0))
		map->seg[i].f = expv; 
	else
		error("Invalid file offset"); 
	if (rdc()=='?' && map == cormap) {
		if (fcor)
			close(fcor);
		fcor=fsym;
		corfil=symfil;
		cormap = symmap;
	} else if (lastc == '/' && map == symmap) {
		if (fsym)
			close(fsym);
		fsym=fcor;
		symfil=corfil;
		symmap=cormap;
	} else
		reread();
}

static int
getfile(char *filnam, int cnt, int omode)
{
	int f;

	if (filnam == 0)
		return -1;
	if (strcmp(filnam, "-") == 0)
		return 0;
	f = open(filnam, omode|OCEXEC);
	if(f < 0 && omode == ORDWR){
		f = open(filnam, OREAD|OCEXEC);
		if(f >= 0)
			dprint("%s open read-only\n", filnam);
	}
	if (f < 0 && xargc > cnt)
		if (wtflag)
			f = create(filnam, 1, 0666);
	if (f < 0) {
		dprint("cannot open `%s': %r\n", filnam);
		return -1;
	}
	return f;
}

void
kmsys(void)
{
	int i;

	i = findseg(symmap, "text");
	if (i >= 0) {
		symmap->seg[i].b = symmap->seg[i].b&~mach->ktmask;
		symmap->seg[i].e = ~0;
	}
	i = findseg(symmap, "data");
	if (i >= 0) {
		symmap->seg[i].b |= mach->kbase;
		symmap->seg[i].e |= mach->kbase;
	}
}

void
attachprocess(void)
{
	char buf[100];
	Dir *sym, *mem;
	int fd;

	if (!adrflg) {
		dprint("used pid$a\n");
		return;
	}
	sym = dirfstat(fsym);
	sprint(buf, "/proc/%lud/mem", adrval);
	corfil = buf;
	setcor();
	sprint(buf, "/proc/%lud/text", adrval);
	fd = open(buf, OREAD);
	mem = nil;
	if (sym==nil || fd < 0 || (mem=dirfstat(fd))==nil
				|| sym->qid.path != mem->qid.path)
		dprint("warning: text images may be inconsistent\n");
	free(sym);
	free(mem);
	if (fd >= 0)
		close(fd);
}
