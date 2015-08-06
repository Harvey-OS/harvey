/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
 *  Performance counters non port part
 */

#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"
#include	"amd64.h"
#include	"../port/pmc.h"


/* non portable, for intel will be CPUID.0AH.EDX
 */

enum {
	PeNreg		= 4,	/* Number of Pe/Pct regs */
};

int
pmcnregs(void)
{
	/* could run CPUID to see if there are registers,
	 * PmcMaxCtrs
 	*/
	return PeNreg;
}

//PeHo|PeGo
#define PeAll	(PeOS|PeUsr)
#define SetEvMsk(v, e) ((v)|(((e)&PeEvMskL)|(((e)<<(PeEvMsksh-8))&PeEvMskH)))
#define SetUMsk(v, u) ((v)|(((u)<<8ull)&PeUnMsk))

#define GetEvMsk(e) (((e)&PeEvMskL)|(((e)&PeEvMskH)>>(PeEvMsksh-8)))
#define GetUMsk(u) (((u)&PeUnMsk)>>8ull)

static int
pmcuserenab(int enable)
{
	uint64_t cr4;

	cr4 = cr4get();
	if (enable){
		cr4 |= Pce;
	} else
		cr4 &=  ~Pce;
	cr4put(cr4);
	return cr4&Pce;
}

PmcCtlCtrId pmcids[] = {
	{"locked instr", "0x024 0x1"},
	{"locked cycles nonspec", "0x024 0x4"},	// cycles
	{"SMI intr", "0x02b 0x0"},
	{"DC access", "0x040 0x0"},
	{"DC miss", "0x041 0x0"},
	{"DC refills", "0x042 0x1f"},
	{"DC evicted", "0x042 0x3f"},
	{"L1 DTLB miss", "0x045 0x7"},		//DTLB L2 hit
	{"L2 DTLB miss", "0x046 0x7"},
	{"L1 DTLB hit", "0x04d 0x3"},
	{"global TLB flush", "0x054 0x0"},
	{"L2 hit", "0x07d 0x3f"},
	{"L2 miss", "0x07e 0xf"},
	{"IC miss", "0x081 0x0"},
	{"IC refill from L2", "0x082 0x0"},
	{"IC refill from system", "0x083 0x0"},
	{"L1 ITLB miss", "0x084 0x0"},			//L2 ITLB hit
	{"L2 ITLB miss", "0x085 0x3"},
	{"DRAM access", "0x0e0 0x3f"},
	{"L3 miss core 0", "0x4e1 0x13"},		//core 0 only
	{"L3 miss core 1", "0x4e1 0x23"},
	{"L3 miss core 2", "0x4e1 0x43"},
	{"L3 miss core 3", "0x4e1 0x83"},
	{"L3 miss socket", "0x4e1 0xf3"},		//all cores in the socket
	{"", ""},
};

int
pmctrans(PmcCtl *p)
{
	PmcCtlCtrId *pi;

	for (pi = &pmcids[0]; pi->portdesc[0] != '\0'; pi++){
		if ( strncmp(p->descstr, pi->portdesc, strlen(pi->portdesc)) == 0){
			strncpy(p->descstr, pi->archdesc, strlen(pi->archdesc) + 1);
			return 0;
		}
	}
	return 1;
}

static int
getctl(PmcCtl *p, uint32_t regno)
{
	uint64_t r, e, u;

	r = rdmsr(regno + PerfEvtbase);
	p->enab = (r&PeCtEna) != 0;
	p->user = (r&PeUsr) != 0;
	p->os = (r&PeOS) != 0;
	e = GetEvMsk(r);
	u = GetUMsk(r);
	//TODO inverse translation
	snprint(p->descstr, KNAMELEN, "%#ullx %#ullx", e, u);
	p->nodesc = 0;
	return 0;
}

int
pmcanyenab(void)
{
	int i;
	PmcCtl p;

	for (i = 0; i < pmcnregs(); i++) {
		if (getctl(&p, i) < 0)
			return -1;
		if (p.enab)
			return 1;
	}

	return 0;
}

extern int pmcdebug;

static int
setctl(PmcCtl *p, int regno)
{
	uint64_t v, e, u;
	char *toks[2];
	char str[KNAMELEN];

	if (regno >= pmcnregs())
		error("invalid reg");

	v = rdmsr(regno + PerfEvtbase);
	v &= PeEvMskH|PeEvMskL|PeCtEna|PeOS|PeUsr|PeUnMsk;
	if (p->enab != PmcCtlNullval)
		if (p->enab)
			v |= PeCtEna;
		else
			v &= ~PeCtEna;

	if (p->user != PmcCtlNullval)
		if (p->user)
			v |= PeUsr;
		else
			v &= ~PeUsr;

	if (p->os != PmcCtlNullval)
		if (p->os)
			v |= PeOS;
		else
			v &= ~PeOS;

	if (pmctrans(p) < 0)
		return -1;

	if (p->nodesc == 0) {
		memmove(str, p->descstr, KNAMELEN);
		if (tokenize(str, toks, 2) != 2)
			return -1;
		e = atoi(toks[0]);
		u = atoi(toks[1]);
		v &= ~(PeEvMskL|PeEvMskH|PeUnMsk);
		v |= SetEvMsk(v, e);
		v |= SetUMsk(v, u);
	}
	if (p->reset != PmcCtlNullval && p->reset) {
		v = 0;
		wrmsr(regno+ PerfCtrbase, 0);
		p->reset = PmcCtlNullval; /* only reset once */
	}
	wrmsr(regno+ PerfEvtbase, v);
	pmcuserenab(pmcanyenab());
	if (pmcdebug) {
		v = rdmsr(regno+ PerfEvtbase);
		print("conf pmc[%#ux]: %#llux\n", regno, v);
	}
	return 0;
}

int
pmcctlstr(char *str, int nstr, PmcCtl *p)
{
	int ns;

	ns = 0;
	if (p->enab && p->enab != PmcCtlNullval)
		ns += snprint(str + ns, nstr - ns, "enable\n");
	else
		ns += snprint(str + ns, nstr - ns, "disable\n");

	if (p->user && p->user != PmcCtlNullval)
		ns += snprint(str + ns, nstr - ns, "user\n");
	if (p->os && p->user != PmcCtlNullval)
		ns += snprint(str + ns, nstr - ns, "os\n");

	//TODO, inverse pmctrans?
	if(!p->nodesc)
		ns += snprint(str + ns, nstr - ns, "%s\n", p->descstr);
	else
		ns += snprint(str + ns, nstr - ns, "no desc\n");
	return ns;
}

int
pmcdescstr(char *str, int nstr)
{
	PmcCtlCtrId *pi;
	int ns;

	ns = 0;

	for (pi = &pmcids[0]; pi->portdesc[0] != '\0'; pi++)
		ns += snprint(str + ns, nstr - ns, "%s\n",pi->portdesc);
	return ns;
}

static uint64_t
getctr(uint32_t regno)
{
	return rdmsr(regno + PerfCtrbase);
}

static int
setctr(uint64_t v, uint32_t regno)
{
	wrmsr(regno + PerfCtrbase, v);
	return 0;
}

static int
notstale(void *x)
{
	PmcCtr *p;
	p = (PmcCtr *)x;
	return !p->stale;
}

static PmcWait*
newpmcw(void)
{
	PmcWait *w;

	w = malloc(sizeof (PmcWait));
	w->ref = 1;
	return w;
}

static void
pmcwclose(PmcWait *w)
{
	if(decref(w))
		return;

	free(w);
}

/*
 *	As it is now, it sends an IPI if the processor is otherwise
 *	ocuppied for it to update the counter.  Probably not needed
 *	for TC/XC as it will be updated every time we cross the kernel
 *	boundary, but we are doing it now just in case it is idle or
 *	not being updated NB: this function releases the ilock
 */

static void
waitnotstale(Mach *mp, PmcCtr *p)
{
	Proc *up = externup();
	PmcWait *w;

	p->stale = 1;
	w = newpmcw();
	w->next = p->wq;
	p->wq = w;
	incref(w);
	iunlock(&(&mp->pmclock)->lock);
	apicipi(mp->apicno);
	if(waserror()){
		pmcwclose(w);
		nexterror();
	}
	sleep(&w->r, notstale, p);
	poperror();
	pmcwclose(w);
}

/*
 *	The reason this is not racy is subtle.
 *
 *	If the processor suddenly changes state to busy once I have
 *	decided not to IPI it, I don't wait for it.
 *
 *	In the other case, I have decided to IPI it and hence, wait.
 *	The problem then is that it switches to idle (not
 *	interruptible) and I wait forever but this switch crosses
 *	kernel boundaries and gets the pmclock.  One of us gets there
 *	first and either I never sleep (p->stale iscleared) or I sleep
 *	and get waken after.  pmclock + rendez locks make sure this is
 *	the case.
 */
static int
shouldipi(Mach *mp)
{
	if(!mp->online)
		return 0;

	if(mp->proc == nil && mp->nixtype == NIXAC)
		return 0;

	return 1;
}

uint64_t
pmcgetctr(uint32_t coreno, uint32_t regno)
{
	Proc *up = externup();
	PmcCtr *p;
	Mach *mp;
	uint64_t v;

	if(coreno == machp()->machno){
		v = getctr(regno);
		if (pmcdebug) {
			print("int getctr[%#ux, %#ux] = %#llux\n", regno, coreno, v);
		}
		return v;
	}

	mp = sys->machptr[coreno];
	p = &mp->pmc[regno];
	ilock(&(&mp->pmclock)->lock);
	p->ctrset |= PmcGet;
	if(shouldipi(mp)){
		waitnotstale(mp, p);
		ilock(&(&mp->pmclock)->lock);
	}
	v = p->ctr;
	iunlock(&(&mp->pmclock)->lock);
	if (pmcdebug) {
		print("ext getctr[%#ux, %#ux] = %#llux\n", regno, coreno, v);
	}
	return v;
}

int
pmcsetctr(uint32_t coreno, uint64_t v, uint32_t regno)
{
	Proc *up = externup();
	PmcCtr *p;
	Mach *mp;

	if(coreno == machp()->machno){
		if (pmcdebug) {
			print("int getctr[%#ux, %#ux] = %#llux\n", regno, coreno, v);
		}
		return setctr(v, regno);
	}

	mp = sys->machptr[coreno];
	p = &mp->pmc[regno];
	if (pmcdebug) {
		print("ext setctr[%#ux, %#ux] = %#llux\n", regno, coreno, v);
	}
	ilock(&(&mp->pmclock)->lock);
	p->ctr = v;
	p->ctrset |= PmcSet;
	if(shouldipi(mp))
		waitnotstale(mp, p);
	else
		iunlock(&(&mp->pmclock)->lock);
	return 0;
}

static void
ctl2ctl(PmcCtl *dctl, PmcCtl *sctl)
{
	if(sctl->enab != PmcCtlNullval)
		dctl->enab = sctl->enab;
	if(sctl->user != PmcCtlNullval)
		dctl->user = sctl->user;
	if(sctl->os != PmcCtlNullval)
		dctl->os = sctl->os;
	if(sctl->nodesc == 0) {
		memmove(dctl->descstr, sctl->descstr, KNAMELEN);
		dctl->nodesc = 0;
	}
}

int
pmcsetctl(uint32_t coreno, PmcCtl *pctl, uint32_t regno)
{
	Proc *up = externup();
	PmcCtr *p;
	Mach *mp;

	if(coreno == machp()->machno)
		return setctl(pctl, regno);

	mp = sys->machptr[coreno];
	p = &mp->pmc[regno];
	ilock(&(&mp->pmclock)->lock);
	ctl2ctl(&p->PmcCtl, pctl);
	p->ctlset |= PmcSet;
	if(shouldipi(mp))
		waitnotstale(mp, p);
	else
		iunlock(&(&mp->pmclock)->lock);
	return 0;
}

int
pmcgetctl(uint32_t coreno, PmcCtl *pctl, uint32_t regno)
{
	Proc *up = externup();
	PmcCtr *p;
	Mach *mp;

	if(coreno == machp()->machno)
		return getctl(pctl, regno);

	mp = sys->machptr[coreno];
	p = &mp->pmc[regno];

	ilock(&(&mp->pmclock)->lock);
	p->ctlset |= PmcGet;
	if(shouldipi(mp)){
		waitnotstale(mp, p);
		ilock(&(&mp->pmclock)->lock);
	}
	memmove(pctl, &p->PmcCtl, sizeof(PmcCtl));
	iunlock(&(&mp->pmclock)->lock);
	return 0;
}

void
pmcupdate(Mach *m)
{
	PmcCtr *p;
	int i, maxct, wk;
	PmcWait *w;

	return;
	maxct = pmcnregs();
	for (i = 0; i < maxct; i++) {
		p = &m->pmc[i];
		ilock(&(&m->pmclock)->lock);
		if(p->ctrset & PmcSet)
			setctr(p->ctr, i);
		if(p->ctlset & PmcSet)
			setctl(p, i);
		p->ctr = getctr(i);
		getctl(p, i);
		p->ctrset = PmcIgn;
		p->ctlset = PmcIgn;
		wk = p->stale;
		p->stale = 0;
		if(wk){
			for(w = p->wq; w != nil; w = w->next){
				p->wq = w->next;
				wakeup(&w->r);
				pmcwclose(w);
			}
		}
		iunlock(&(&m->pmclock)->lock);
	}
}

