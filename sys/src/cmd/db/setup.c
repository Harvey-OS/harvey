
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

extern	Mach		mmips;
extern	Machdata	mipsmach;

static int getfile(char*, int, int);

void
setsym(void)
{
	Symbol s;

	if((fsym = getfile(symfil, 1, wtflag)) < 0) {
		symmap = dumbmap(-1);
		machdata = &mipsmach;
		mach = &mmips;
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
	char buf[64];
	int fd;
	ulong ksp;
	long proc;		/* address of proc table */


	if(pid > 0){	/* set kbase; should probably do other things, too */
		sprint(buf, "/proc/%d/proc", pid);
		fd = open(buf, 0);
		if(fd >= 0){
			seek(fd, mach->kspoff, 0);
			if(read(fd, (char *)&ksp, 4) == 4)
 			mach->kbase = machdata->swal(ksp) & ~(mach->pgsize-1);
			close(fd);
		}
	}
	if (cormap)
		close(fcor);

	fcor = getfile(corfil, 2, ORDWR);
	if (fcor <= 0) {
		if (cormap)
			free(cormap);
		cormap = dumbmap(-1);
		return;
	}
	cormap = newmap(cormap, fcor, 3);
	if (!cormap)
		return;
	setmap(cormap, fhdr.txtaddr, fhdr.txtaddr+fhdr.txtsz, fhdr.txtaddr, "text");
	setmap(cormap, fhdr.dataddr, mach->kbase, fhdr.dataddr, "data");
	setmap(cormap, mach->kbase, mach->kbase+mach->pgsize, mach->kbase, "ublock");
	fixregs(cormap);
	if (kflag) {
		if (get4(cormap, mach->kbase, &proc) < 0)
			error("can't find proc table: %r");

		adjustreg(mach->pc, proc+mach->kpcoff, mach->kpcdelta);
		adjustreg(mach->sp, proc+mach->kspoff, mach->kspdelta);
	}
	kmsys();
	return;
}

Map *
dumbmap(int fd)
{
	Map *dumb;
	extern Mach mmips;
	extern Machdata mipsmach;

	dumb = newmap(0, fd, 1);
	setmap(dumb, 0, 0xffffffff, 0, "data");
	if (!mach) 			/* default machine = mips */
		mach = &mmips;
	if (!machdata)
		machdata = &mipsmach;
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
	i = findseg(symmap, "ublock");
	if (i >= 0)
		unusemap(symmap, i);
	cormap = newmap(cormap, fcor, 1);
	if (cormap)
		setmap(cormap, 0, ~0, 0, "data");
}

void
attachproc(void)
{
	char buf[100];
	int statstat;
	Dir sym, mem;
	int fd;

	if (!adrflg) {
		dprint("used pid$a\n");
		return;
	}
	statstat = dirfstat(fsym, &sym);
	sprint(buf, "/proc/%d/mem", adrval);
	corfil = buf;
	setcor();
	sprint(buf, "/proc/%d/text", adrval);
	fd = open(buf, OREAD);
	if (statstat < 0 || fd < 0 || dirfstat(fd, &mem) < 0
				|| sym.qid.path != mem.qid.path)
		dprint("warning: text images may be inconsistent\n");
	if (fd >= 0)
		close(fd);
}
