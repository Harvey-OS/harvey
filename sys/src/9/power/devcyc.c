#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"
#include	"devtab.h"

#include	"io.h"
#include	"../../fs/cyc/comm.h"


typedef struct Commcyc	Commcyc;

enum
{
	Vmevec=		0xd2,		/* vme vector for interrupts */
	Intlevel=	5,		/* level to interrupt on */
	Qdir=		0,		/* Qid's */
	Qcyc=		1,
	Ncyc=		1,
};

Dirtab cycdir[]={
	"cyc",	{Qcyc},	0,	0600,
};

#define	NCYCDIR	(sizeof cycdir/sizeof(Dirtab))

struct Commcyc
{
	Lock;
	QLock		buflock;
	Lock		busy;
	Comm		*addr;		/* address of the device */
	int		vec;		/* vme interrupt vector */
	int		wi;		/* where to write next cmd */
	int		ri;		/* where to read next reply */
	uchar		buf[MAXFDATA+MAXMSG];
};

Commcyc cyclone[Ncyc];

void	cycintr(int);

#define	CYCLONE	VMEA24SUP(Comm, 0x10000);

/*
 * Commands
 */

Cycmsg**
cycsend(Commcyc *h, Cycmsg *m)
{
	Cycmsg **mp;

	lock(h);
	mp = (Cycmsg**)&h->addr->reqstq[h->wi];
	*mp = (Cycmsg*)MP2VME(m);
	h->wi++;
	if(h->wi >= NRQ)
		h->wi = 0;
	unlock(h);
	return mp;
}

void
cycwait(Cycmsg **mp)
{
	ulong l;

	l = 0;
	while(*mp){
		delay(0);	/* just a subroutine call; stay off VME */
		if(++l > 10*1000*1000){
			l = 0;
			panic("cycsend blocked");
		}
	}
	return;
}

/*
 *  reset all cyclone boards
 */
void
cycreset(void)
{
	int i;
	Commcyc *cp;

	for(cp=cyclone,i=0; i<Ncyc; i++,cp++){
		cp->addr = CYCLONE+i;
		/*
		 * Write queue is at end of cyclone memory
		 */
		cp->vec = Vmevec+i;
		setvmevec(cp->vec, cycintr);
	}	
}

void
cycinit(void)
{
}

/*
 *  enable the device for interrupts, spec is the device number
 */
Chan*
cycattach(char *spec)
{
	int i;
	Chan *c;

	i = strtoul(spec, 0, 0);
	if(i >= Ncyc)
		error(Ebadarg);

	c = devattach('C', spec);
	c->dev = i;
	c->qid.path = CHDIR;
	c->qid.vers = 0;
	return c;
}

Chan*
cycclone(Chan *c, Chan *nc)
{
	return devclone(c, nc);
}

int	 
cycwalk(Chan *c, char *name)
{
	return devwalk(c, name, cycdir, NCYCDIR, devgen);
}

void	 
cycstat(Chan *c, char *dp)
{
	devstat(c, dp, cycdir, NCYCDIR, devgen);
}

Chan*
cycopen(Chan *c, int omode)
{
	Commcyc *cp;
	Cycmsg *mp, **cmp;

	if(c->qid.path == CHDIR){
		if(omode != OREAD)
			error(Eperm);
	}else if(c->qid.path == Qcyc){
		cp = &cyclone[c->dev];
		if(!canlock(&cp->busy))
			error(Einuse);
		/*
		 * Clear communications region
		 */
		memset(cp->addr->reqstq, 0, NRQ*sizeof(ulong));
		cp->addr->reqstp = 0;
		memset(cp->addr->replyq, 0, NRQ*sizeof(ulong));
		cp->addr->replyp = 0;

		/* Set the interrupt vector for the cyclone to pick up */
		cp->addr->vmevec = Vmevec;

		/*
		 * Issue reset
		 */
		cp->wi = 0;
		cp->ri = 0;
		mp = &u->kcyc;
		mp->cmd = Ureset;
		cmp = cycsend(cp, &((User*)(u->p->upage->pa|KZERO))->kcyc);
		cycwait(cmp);
		delay(100);
		print("reset\n");
	}
	c->mode = openmode(omode);
	c->flag |= COPEN;
	c->offset = 0;
	return c;
}

void	 
cyccreate(Chan *c, char *name, int omode, ulong perm)
{
	USED(c, name, omode, perm);
	error(Eperm);
}

void	 
cycclose(Chan *c)
{
	Commcyc *cp;
	Cycmsg **cmp;

	cp = &cyclone[c->dev];
	if(c->qid.path != CHDIR){
		u->kcyc.cmd = Ureboot;
		cmp = cycsend(cp, &((User*)(u->p->upage->pa|KZERO))->kcyc);
		cycwait(cmp);
		unlock(&cp->busy);
	}
}

int
cycmsgintr(Cycmsg *hm)
{
	return hm->intr;
}

/*
 * Read and write use physical addresses if they can, which they usually can.
 * Most I/O is from devmnt, which has local buffers.  Therefore just check
 * that buf is in KSEG0 and is at an even address.
 */

long	 
cycread(Chan *c, void *buf, long n, ulong offset)
{
	User *pu;
	Commcyc *cp;
	Cycmsg *mp, **cmp;
	ulong l, m;

	USED(offset);
	cp = &cyclone[c->dev];
	switch(c->qid.path & ~CHDIR){
	case Qdir:
		return devdirread(c, buf, n, cycdir, NCYCDIR, devgen);

	case Qcyc:
		if(n > sizeof cp->buf){
			print("cyclone bufsize %d %d\n", n, sizeof cp->buf);
			error(Egreg);
		}
		pu = (User*)(u->p->upage->pa|KZERO);
		if((((ulong)buf)&(KSEGM|3)) == KSEG0){
			/*
			 *  use supplied buffer, no need to lock for reply
			 */

			mp = &pu->kcyc;
			mp->cmd = Uread;
			mp->param[0] = MP2VME(buf);
			mp->param[1] = n;
			mp->param[2] = 0;		/* reply checksum */
			mp->param[3] = 0xdeadbeef;	/* reply count */
			mp->intr = 0;
			cycsend(cp, mp);

			while(waserror())
				;
			sleep(&mp->r, cycmsgintr, mp);
			poperror();

			m = mp->param[3];
			if(m==0 || m>n){
				print("devcyc: count 0x%lux 0x%lux\n", m, n);
				error(Egreg);
			}
		}
		else{
			/*
			 * use cyclone buffer. lock the buffer until the reply
			 */
			mp = &pu->ucyc;
			qlock(&cp->buflock);
			mp->cmd = Uread;
			mp->param[0] = MP2VME(cp->buf);
			mp->param[1] = n;
			mp->param[2] = 0;	/* reply checksum */
			mp->param[3] = 0;	/* reply count */
			mp->intr = 0;
			cmp = cycsend(cp, mp);
			cycwait(cmp);
			l = 100*1000*1000;
			do
				m = mp->param[3];
			while(m==0 && --l>0);
			if(m==0 || m>n){
				print("devcyc: count %ld %ld\n", m, n);
				qunlock(&cp->buflock);
				error(Egreg);
			}
			memmove(buf, cp->buf, m);
			qunlock(&cp->buflock);
		}
		return m;
	}
	print("devcyc read unk\n");
	error(Egreg);
	return 0;
}

long	 
cycwrite(Chan *c, void *buf, long n, ulong offset)
{
	User *pu;
	Commcyc *cp;
	Cycmsg *mp, **cmp;

	USED(offset);
	cp = &cyclone[c->dev];
	switch(c->qid.path & ~CHDIR){
	case Qdir:
		return devdirread(c, buf, n, cycdir, NCYCDIR, devgen);

	case Qcyc:
		if(n > sizeof cp->buf){
			print("cyclone write bufsize\n");
			error(Egreg);
		}
		pu = (User*)(u->p->upage->pa|KZERO);
		if((((ulong)buf)&(KSEGM|3)) == KSEG0){
			/*
			 * use supplied buffer, no need to lock for reply
			 */
			mp = &pu->kcyc;

			mp->cmd = Uwrite;
			mp->param[0] = MP2VME(buf);
			mp->param[1] = n;
			mp->param[2] = 0;

			cmp = cycsend(cp, mp);
			cycwait(cmp);
		}else{
			/*
			 * use cyclone buffer.  lock the buffer until the reply
			 */
			mp = &pu->ucyc;
			qlock(&cp->buflock);
			memmove(cp->buf, buf, n);

			mp->cmd = Uwrite;
			mp->param[0] = MP2VME(cp->buf);
			mp->param[1] = n;
			mp->param[2] = 0;

			cmp = cycsend(cp, mp);
			cycwait(cmp);
			qunlock(&cp->buflock);
		}
		return n;
	}
	print("cyclone write unk\n");
	error(Egreg);
	return 0;
}

void	 
cycremove(Chan *c)
{
	USED(c);
	error(Eperm);
}

void	 
cycwstat(Chan *c, char *dp)
{
	USED(c, dp);
	error(Eperm);
}

void
cycintr(int vec)
{
	Cycmsg *cm;
	Commcyc *h;
	ulong l, n, *r;

	h = &cyclone[vec - Vmevec];
	if(h<cyclone || h>&cyclone[Ncyc]){
		print("bad cyclone vec\n");
		return;
	}

	r = h->addr->replyq;

	for(;;) {
		l = r[h->ri];
		if(l == 0)
			break;

		r[h->ri++] = 0;
		if(h->ri >= NRQ)
			h->ri = 0;

		cm = (Cycmsg*)VME2MP(l);
		cm->intr = 1;
		n = cm->param[3];
		if(n==0 || n > 10000)
			print("cycintr count 0x%lux\n", n);

		wakeup(&cm->r);
	}
}
