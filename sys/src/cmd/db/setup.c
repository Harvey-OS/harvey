/*
 * init routines
 */
#include "defs.h"
#include "fns.h"



/*
 *	some default files
 */
char	*symfil	= "v.out";
char	*corfil	= 0;


Map	*symmap;
Map	*cormap;

int fsym, fcor;

static int getfile(char *, int, int);

void
setsym(void)
{
	Fhdr f;
	Symbol s;

	if((fsym = getfile(symfil, 1, wtflag)) < 0) {
		symmap = dumbmap(-1);
		return;
	}
	if (crackhdr(fsym, &f)) {
		var[varchk('m')] = f.magic;
		var[varchk('e')] = f.entry;
		var[varchk('t')] = f.txtsz;
		var[varchk('b')] = f.dataddr;
		var[varchk('d')] = f.datsz;
		machbytype(f.type);
		symmap = loadmap(symmap, fsym, &f);
		if (symmap == 0)
			symmap = dumbmap(fsym);
		if (syminit(fsym, &f) < 0)
			if (symerror)
				dprint("%s\n", symerror);
		if (mach->sbreg && lookup(0, mach->sbreg, &s))
			mach->sb = s.value;
	}
	else
		symmap = dumbmap(fsym);
}
int
mapcore(void)
{
	return 0;
}
int
mapimage(void)
{
	if(pid > 0){	/* set kbase; should probably do other things, too */
		char buf[64];
		int fd;
		ulong ksp;

		sprint(buf, "/proc/%d/proc", pid);
		fd = open(buf, 0);
		if(fd >= 0){
			seek(fd, mach->kspoff, 0);
			if(read(fd, (char *)&ksp, 4L) == 4)
				mach->kbase = machdata->swal(ksp) & ~(mach->pgsize-1);

			close(fd);
		}
	}

	cormap = newmap(cormap, fcor);
	if (cormap == 0)
		return 0;
	setmap(cormap, SEGDATA, mach->pgsize, mach->kbase & ~mach->ktmask, mach->pgsize);
	setmap(cormap, SEGUBLK, mach->kbase, mach->kbase+mach->pgsize, mach->kbase);
	setmap(cormap, SEGREGS, 0, mach->pgsize, mach->kbase);
	machdata->rsnarf(mach->reglist);
	if(kflag)
		kmprocset(pid);
	return (1);

}

void
setcor(void)
{
	fcor = getfile(corfil, 2, ORDWR);
	if (fcor <= 0 || (mapimage() == 0 && mapcore() == 0))
		cormap = dumbmap(fcor);
}

Map *
dumbmap(int fd)
{
	Map *dumb;
	extern	Mach	mmips;

	dumb = newmap(0, fd);
	var[varchk('b')] = 0;
	var[varchk('d')] = 0xffffffff;
	unusemap(dumb, SEGTEXT);
	setmap(dumb, SEGDATA, 0, 0xffffffff,0);
	unusemap(dumb, SEGUBLK);
	unusemap(dumb, SEGREGS);
	if (!mach) 			/* default machine = mips */
		mach = &mmips;
	return dumb;
}

/*
 * set up maps for a direct process image (/proc)
 */

void
cmdmap(Map *map)
{
	int i;
	ulong b, e, f;
	char name[MAXSYM];

	extern char lastc;

	rdc();
	readsym(name);
	for (i = 0; i < MAXSEGS; i++)
		if (strcmp(map->seg[i].name, name) == 0)
			break;
	if (i >= MAXSEGS)	/* not found */
		error("Invalid map name");
	b = e = f = 0;		/* to shut compiler up */
	if (expr(0))
		b = expv;
	else
		error("Invalid base address"); 
	if (expr(0))
		e = expv;
	else
		error("Invalid end address"); 
	if (expr(0))
		f = expv; 
	else
		error("Invalid file offset"); 
	setmap(map, i, b, e, f);
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
	setmap(symmap, SEGTEXT,symmap->seg[SEGTEXT].b &~mach->ktmask,
				~0,
				symmap->seg[SEGTEXT].f);
	setmap(symmap, SEGDATA, symmap->seg[SEGDATA].b | mach->kbase,
				symmap->seg[SEGDATA].e | mach->kbase,
				symmap->seg[SEGDATA].f);
	unusemap(symmap, SEGUBLK);
	unusemap(symmap, SEGREGS);
	cormap = newmap(cormap, fcor);
	if (cormap) {
		unusemap(cormap, SEGTEXT);
		setmap(cormap, SEGDATA, 0, ~0, 0);
		unusemap(cormap, SEGUBLK);
		unusemap(cormap, SEGREGS);
	}
}

void
kmprocset(long pid)
{
	char buf[20];
	int f;
	long ksp, kpc;

	sprint(buf, "/proc/%d/proc", pid);
	f = open(buf, 0);
	if (f < 0) {
		dprint("can't open %s\n", buf);
		return;
	}
	seek(f, mach->kpcoff, 0);
	if(read(f, (char *)&kpc, 4L) != 4){
    no:
		dprint("kpc/ksp read error\n");
		close(f);
		return;
	}
	seek(f, mach->kspoff, 0);
	if(read(f, (char *)&ksp, 4L) != 4)
		goto no;
	rput(mach->pc, kpc+mach->kpcdelta);
	rput(mach->sp, ksp+mach->kspdelta);
	regdirty = 0;
	close(f);
	kmsys();
}

void
kmproc(void)
{
	if (!adrflg) {
		dprint("use pid$p\n");
		return;
	}
	kmprocset(adrval);
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
	if (cormap)
		close(fcor);
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
