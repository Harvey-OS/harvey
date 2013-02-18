#include	<u.h>
#include	<libc.h>
#include	<tos.h>

extern	long	_callpc(void**);
extern	long	_savearg(void);

static	ulong	khz;
static	ulong	perr;
static	int	havecycles;

typedef	struct	Plink	Plink;
struct	Plink
{
	Plink	*old;
	Plink	*down;
	Plink	*link;
	long	pc;
	long	count;
	vlong time;
};

#pragma profile off

ulong
_profin(void)
{
	void *dummy;
	long pc;
	Plink *pp, *p;
	ulong arg;
	vlong t;

	arg = _savearg();
	pc = _callpc(&dummy);
	pp = _tos->prof.pp;
	if(pp == 0 || (_tos->prof.pid && _tos->pid != _tos->prof.pid))
		return arg;

	for(p=pp->down; p; p=p->link)
		if(p->pc == pc)
			goto out;
	p = _tos->prof.next + 1;
	if(p >= _tos->prof.last) {
		_tos->prof.pp = 0;
		perr++;
		return arg;
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
		p->time = p->time - _tos->pcycles;
		goto proftime;
	case Profuser:
		/* Add kernel cycles on proc entry */
		p->time = p->time + _tos->kcycles;
		/* fall through */
	case Proftime:	
	proftime:		/* Subtract cycle counter on proc entry */
		cycles((uvlong*)&t);
		p->time = p->time - t;
		break;
	case Profsample:
		p->time = p->time - _tos->clock;
		break;
	}
	return arg;		/* disgusting linkage */
}

ulong
_profout(void)
{
	Plink *p;
	ulong arg;
	vlong t;

	arg = _savearg();
	p = _tos->prof.pp;
	if (p == nil || (_tos->prof.pid != 0 && _tos->pid != _tos->prof.pid))
		return arg;	/* Not our process */
	switch(_tos->prof.what){
	case Profkernel:		/* Add proc cycles on proc entry */
		p->time = p->time + _tos->pcycles;
		goto proftime;
	case Profuser:			/* Subtract kernel cycles on proc entry */
		p->time = p->time - _tos->kcycles;
		/* fall through */
	case Proftime:	
	proftime:				/* Add cycle counter on proc entry */
		cycles((uvlong*)&t);
		p->time = p->time + t;
		break;
	case Profsample:
		p->time = p->time + _tos->clock;
		break;
	}
	_tos->prof.pp = p->old;
	return arg;
}

void
_profdump(void)
{
	int f;
	long n;
	Plink *p;
	char *vp;
	char filename[64];

	if (_tos->prof.what == 0)
		return;	/* No profiling */
	if (_tos->prof.pid != 0 && _tos->pid != _tos->prof.pid)
		return;	/* Not our process */
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
	_tos->prof.pid = ~0;	/* make sure data gets dumped once */
	switch(_tos->prof.what){
	case Profkernel:
		cycles((uvlong*)&_tos->prof.first->time);
		_tos->prof.first->time = _tos->prof.first->time + _tos->pcycles;
		break;
	case Profuser:
		cycles((uvlong*)&_tos->prof.first->time);
		_tos->prof.first->time = _tos->prof.first->time - _tos->kcycles;
		break;
	case Proftime:
		cycles((uvlong*)&_tos->prof.first->time);
		break;
	case Profsample:
		_tos->prof.first->time = _tos->clock;
		break;
	}
	vp = (char*)_tos->prof.first;

	for(p = _tos->prof.first; p <= _tos->prof.next; p++) {

		/*
		 * short down
		 */
		n = 0xffff;
		if(p->down)
			n = p->down - _tos->prof.first;
		vp[0] = n>>8;
		vp[1] = n;

		/*
		 * short right
		 */
		n = 0xffff;
		if(p->link)
			n = p->link - _tos->prof.first;
		vp[2] = n>>8;
		vp[3] = n;
		vp += 4;

		/*
		 * long pc
		 */
		n = p->pc;
		vp[0] = n>>24;
		vp[1] = n>>16;
		vp[2] = n>>8;
		vp[3] = n;
		vp += 4;

		/*
		 * long count
		 */
		n = p->count;
		vp[0] = n>>24;
		vp[1] = n>>16;
		vp[2] = n>>8;
		vp[3] = n;
		vp += 4;

		/*
		 * vlong time
		 */
		if (havecycles){
			n = (vlong)(p->time / (vlong)khz);
		}else
			n = p->time;

		vp[0] = n>>24;
		vp[1] = n>>16;
		vp[2] = n>>8;
		vp[3] = n;
		vp += 4;
	}
	write(f, (char*)_tos->prof.first, vp - (char*)_tos->prof.first);
	close(f);
}

void
_profinit(int entries, int what)
{
	if (_tos->prof.what == 0)
		return;	/* Profiling not linked in */
	_tos->prof.pp = nil;
	_tos->prof.first = mallocz(entries*sizeof(Plink),1);
	_tos->prof.last = _tos->prof.first + entries;
	_tos->prof.next = _tos->prof.first;
	_tos->prof.pid = _tos->pid;
	_tos->prof.what = what;
	_tos->clock = 1;
}

void
_profmain(void)
{
	char ename[50];
	int n, f;

	n = 2000;
	if (_tos->cyclefreq != 0LL){
		khz = _tos->cyclefreq / 1000;	/* Report times in milliseconds */
		havecycles = 1;
	}
	f = open("/env/profsize", OREAD);
	if(f >= 0) {
		memset(ename, 0, sizeof(ename));
		read(f, ename, sizeof(ename)-1);
		close(f);
		n = atol(ename);
	}
	_tos->prof.what = Profuser;
	f = open("/env/proftype", OREAD);
	if(f >= 0) {
		memset(ename, 0, sizeof(ename));
		read(f, ename, sizeof(ename)-1);
		close(f);
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

void prof(void (*fn)(void*), void *arg, int entries, int what)
{
	_profinit(entries, what);
	_tos->prof.pp = _tos->prof.next;
	fn(arg);
	_profdump();
}

#pragma profile on

