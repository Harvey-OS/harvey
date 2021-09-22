#include	<u.h>
#include	<libc.h>
#include	<tos.h>

/*
 * profiling uses ulong where uintptr would be ideal, including in the prof.out
 * format.  we'll just have to hope that we can squeeze each text segment into
 * a mere 4GB.
 */
extern	ulong	_callpc(void**);	/* should return uintptr ideally */
extern	uintptr	_savearg(void), _saveret(void);

static	ulong	khz;
static	ulong	perr;
static	int	havecycles;

typedef	struct	Plink	Plink;
struct	Plink
{
	Plink	*old;
	Plink	*down;
	Plink	*link;
	long	pc;		/* unportable, but see prof.out format */
	long	count;
	vlong	time;
};

#pragma profile off

/*
 * on some machines (e.g., amd64), the RARG and return value registers are
 * different, so restore the old RARG into RARG by calling this function.
 * part of the disgusting linkage.
 */
static void
pushrarg(uintptr oldrarg)
{
	USED(oldrarg);
}

void
_profin(void)
{
	void *dummy;
	Plink *pp, *p;
	long pc;		/* unportable, but see prof.out format */
	uintptr arg;
	vlong t;

	/* save RARG of caller; regs other than SP have been saved by caller's caller */
	arg = _savearg();
	pc = _callpc(&dummy);
	pp = _tos->prof.pp;
	if(pp == 0 || (_tos->prof.pid && _tos->pid != _tos->prof.pid)) {
		pushrarg(arg);
		return;
	}

	for(p=pp->down; p; p=p->link)
		if(p->pc == pc)
			goto out;
	p = _tos->prof.next + 1;
	if(p >= _tos->prof.last) {
		_tos->prof.pp = 0;
		perr++;
		pushrarg(arg);
		return;
	}
	_tos->prof.next = p;
	p->link = pp->down;
	pp->down = p;
	p->pc = pc;
	p->old = pp;
	p->down = 0;
	p->count = 0;
	p->time = 0LL;

out:
	_tos->prof.pp = p;
	p->count++;
	switch(_tos->prof.what){
	case Profkernel:
		p->time -= _tos->pcycles;
		goto proftime;
	case Profuser:
		/* Add kernel cycles on proc entry */
		p->time += _tos->kcycles;
		/* fall through */
	case Proftime:	
	proftime:		/* Subtract cycle counter on proc entry */
		t = 0;
		cycles((uvlong*)&t);
		p->time -= t;
		break;
	case Profsample:
		p->time -= _tos->clock;
		break;
	}
	pushrarg(arg);			/* disgusting linkage: restore RARG */
}

uintptr
_profout(void)
{
	Plink *p;
	uintptr arg;
	vlong t;

	arg = _saveret();		/* save return reg of caller */
	p = _tos->prof.pp;
	if (p == nil || (_tos->prof.pid != 0 && _tos->pid != _tos->prof.pid))
		return arg;		/* Not our process */

	switch(_tos->prof.what){
	case Profkernel:		/* Add proc cycles on proc entry */
		p->time += _tos->pcycles;
		goto proftime;
	case Profuser:			/* Subtract kernel cycles on proc entry */
		p->time -= _tos->kcycles;
		/* fall through */
	case Proftime:	
	proftime:			/* Add cycle counter on proc entry */
		t = 0;
		cycles((uvlong*)&t);
		p->time += t;
		break;
	case Profsample:
		p->time += _tos->clock;
		break;
	}
	_tos->prof.pp = p->old;
	return arg;	/* disgusting linkage: return caller's return reg */
}

static char *
beputl(char *p, ulong n)
{
	p[0] = n>>24;
	p[1] = n>>16;
	p[2] = n>>8;
	p[3] = n;
	return p + 4;
}

void
_profdump(void)
{
	int f;
	long n;
	vlong tm;
	Plink *p;
	char *vp;
	char filename[64];

	if (_tos->prof.what == 0)
		return;			/* No profiling */
	if (_tos->prof.pid != 0 && _tos->pid != _tos->prof.pid)
		return;			/* Not our process */

	if(perr)
		fprint(2, "%lud Prof errors\n", perr);
	_tos->prof.pp = nil;
	if (_tos->prof.pid)
		snprint(filename, sizeof filename - 1, "prof.%ld", _tos->prof.pid);
	else
		snprint(filename, sizeof filename - 1, "prof.out");
	f = create(filename, 1, 0666);
	if(f < 0) {
		perror("create prof.out");
		return;
	}
	_tos->prof.pid = ~0;		/* make sure data gets dumped once */
	if (_tos->prof.what != Profsample) {
		_tos->prof.first->time = 0;
		cycles((uvlong*)&_tos->prof.first->time);
	}
	switch(_tos->prof.what){
	case Profkernel:
		_tos->prof.first->time += _tos->pcycles;
		break;
	case Profuser:
		_tos->prof.first->time -= _tos->kcycles;
		break;
	case Proftime:
		break;
	case Profsample:
		_tos->prof.first->time = _tos->clock;
		break;
	}

	vp = (char*)_tos->prof.first;
	for(p = _tos->prof.first; p <= _tos->prof.next; p++) {
		/*
		 * short down and right
		 */
		n = 0xffff;
		if(p->down)
			n = p->down - _tos->prof.first;
		vp[0] = n>>8;
		vp[1] = n;

		n = 0xffff;
		if(p->link)
			n = p->link - _tos->prof.first;
		vp[2] = n>>8;
		vp[3] = n;
		vp += 4;

		/*
		 * long pc and count
		 */
		vp = beputl(vp, p->pc);
		vp = beputl(vp, p->count);

		/*
		 * vlong time converted to ulong
		 */
		tm = p->time;
		if (havecycles && khz)
			tm /= khz;
		vp = beputl(vp, tm);
	}
	write(f, (char*)_tos->prof.first, vp - (char*)_tos->prof.first);
	close(f);
}

void
_profinit(int entries, int what)
{
	if (_tos->prof.what == 0)
		return;			/* Profiling not linked in */
	_tos->prof.pp = nil;
	_tos->prof.first = mallocz(entries*sizeof(Plink),1);
	_tos->prof.last = _tos->prof.first + entries;
	_tos->prof.next = _tos->prof.first;
	_tos->prof.pid = _tos->pid;
	_tos->prof.what = what;
	_tos->clock = 1;
}

static void
readall(int f, char *buf, int max)
{
	int n;

	n = (max > 1? readn(f, buf, max - 1): 0);
	buf[n >= 0? n: 0] = '\0';
	close(f);
}

void
_profmain(void)
{
	char ename[50];
	int n, f;

	if (_tos->cyclefreq != 0LL){
		khz = _tos->cyclefreq / 1000;	/* Report times in milliseconds */
		havecycles = 1;
	}
	n = 0;
	f = open("/env/profsize", OREAD);
	if(f >= 0) {
		readall(f, ename, sizeof ename);
		n = atol(ename);
	}
	if (n == 0)
		n = 2000;
	_tos->prof.what = Profuser;
	f = open("/env/proftype", OREAD);
	if(f >= 0) {
		readall(f, ename, sizeof ename);
		if (strcmp(ename, "user") == 0)
			_tos->prof.what = Profuser;
		else if (strcmp(ename, "kernel") == 0)
			_tos->prof.what = Profkernel;
		else if (strcmp(ename, "elapsed") == 0 || strcmp(ename, "time") == 0)
			_tos->prof.what = Proftime;
		else if (strcmp(ename, "sample") == 0)
			_tos->prof.what = Profsample;
	}
	_tos->prof.first = sbrk(n*sizeof(Plink));
	_tos->prof.last = sbrk(0);
	_tos->prof.next = _tos->prof.first;
	_tos->prof.pp = nil;
	_tos->prof.pid = _tos->pid;
	atexit(_profdump);
	_tos->clock = 1;
}

void
prof(void (*fn)(void*), void *arg, int entries, int what)
{
	_profinit(entries, what);
	_tos->prof.pp = _tos->prof.next;
	fn(arg);
	_profdump();
}

#pragma profile on

