#include	"all.h"
#include	"../cyc/comm.h"
#include	"io.h"

typedef	struct	Hcmd	Hcmd;
typedef	struct	Hhdr	Hhdr;
typedef	struct	Exec	Exec;
typedef	struct	Out	Out;
typedef struct  Cyclone Cyclone;

enum
{
	SUM	= 0,
	NBUF	= 100,
	NLBUF	= 50,
	NPARAM	= 6,
	NOUT	= 20,
	NHASH	= 290,
	NCYC	= 2,
};

struct	Hhdr
{
	Hcmd*	hcmd;
	int	mbsize;
	Msgbuf*	mb;
	ulong	hcmap;		/* vme map for this Hcmd structure */
	ulong	mbmap;		/* vme map for this mb->data */
};

struct	Hcmd
{
	ulong	cmd;
	ulong	param[NPARAM];
};

struct Out
{
	Hhdr;
	Msgbuf*	mb;
	Hcmd*	hc;
	ulong*	ip;
};

struct Cyclone
{
	Queue*	reply;
	struct
	{
		Vmedevice* vme;
		int	hpactive;
		int	cmdwait;
		int	verb;
		long	oubusy;
		Rendez	hrir;			/* vme interrupt */
		QLock	issueq;
		ulong	issuep;
		ulong	replyp;
		int	bhash[NHASH];
		int	vhash;
		Hhdr	buf[NBUF];		/* outstanding buffers */
		Chan*	chan;
		Out	out[NOUT];

		Filter	rate;
		Filter	count;
	};
};

static Cyclone h[NCYC];

static	int	cyclissue(Cyclone*, Hhdr*, Hcmd*, int);
static	void	cmd_cycl(int, char*[]);
static	void	cmd_stats(int, char*[]);

void
cyclintr(Vmedevice *vp)
{
	int i;
	Cyclone *hp;

	hp = h;
	for(i = 0; i < NCYC; i++) {
		if(vp == hp->vme) {
			hp->cmdwait = 1;
			wakeup(&hp->hrir);
			break;
		}
		hp++;
	}
}

int
cyclinit(Vmedevice *vp)
{
	Hhdr *hc;
	Chan *cp;
	Msgbuf *m;
	Cyclone *hp;
	int i, n, bv, bc, c, ncyc;

	for(ncyc = 0; ncyc < NCYC; ncyc++)
		if(h[ncyc].vme == 0)
			break;

	if(ncyc == NCYC)
		panic("config more cyclones");

	print("cyclinit: %d\n", ncyc);

	hp = &h[ncyc];

	memset(hp, 0, sizeof(*hp));
	hp->vme = vp;
	hp->reply = newqueue(10);
	cp = chaninit(Devcycl, 1);
	hp->chan = cp;
	cp->send = serveq;
	cp->reply = hp->reply;

	for(i=0; i<NBUF; i++) {
		n = LARGEBUF;
		if(i >= NLBUF)
			n = SMALLBUF;
		m = mballoc(n, hp->chan, Mbcycl1);
		hc = &hp->buf[i];
		hc->hcmd = ialloc(sizeof(Hcmd), LINESIZE);
		hc->mb = m;
		hc->mbsize = n;
		hc->mbmap = vme2sysmap(hp->vme->bus, (ulong)m->data, n);
		hc->hcmap = vme2sysmap(hp->vme->bus, (ulong)hc->hcmd, sizeof(Hcmd));
	}

	/*
	 * find best hash divisor
	 */
	bc = NHASH;
	bv = 0;
	for(hp->vhash=10; hp->vhash<=NHASH; hp->vhash++) {
		memset(hp->bhash, 0, sizeof(hp->bhash));
		c = 0;
		for(i=0; i<NBUF; i++) {
			hc = &hp->buf[i];
			n = hc->hcmap%hp->vhash;
			if(hp->bhash[n])
				c++;
			hp->bhash[n] = 1;
		}
		if(c < bc) {
			bc = c;
			bv = hp->vhash;
		}
	}
	print("best v = %d %d collisions\n", bv, bc);
	hp->vhash = bv;
	memset(hp->bhash, 0, sizeof(hp->bhash));
	for(i=0; i<NBUF; i++) {
		hc = &hp->buf[i];
		hp->bhash[hc->hcmap%hp->vhash] = i;
	}

	dofilter(&hp->count);
	dofilter(&hp->rate);

	if(ncyc == 0) {
		cmd_install("cycl", "subcommand -- issue cyclone command", cmd_cycl);
		cmd_install("statc", "-- cyclone stats", cmd_stats);
	}
	return 0;
}

ulong
addsum(void *d, int count)
{
	ulong *p, s;
	int i, n;

	n = (count+3)/4;
	p = d;
	s = 0;
	for(i=0; i<n; i++)
		s += *p++;
	return s;
}

void
cyclou(void)
{
	Hcmd *hc;
	Comm *com;
	Msgbuf *mb;
	Cyclone *hp;
	int i, count;
	Out *op, *uop;

	hp = getarg();

	for(i=0; i<NOUT; i++) {
		op = &hp->out[i];
		op->mb = 0;
		op->hc = ialloc(sizeof(Hcmd), LINESIZE);
		op->hcmap = vme2sysmap(hp->vme->bus, (ulong)op->hc, sizeof(Hcmd));
	}

	print("cyo%d: start\n", hp-h);
	com = hp->vme->address;

loop:
	mb = recv(hp->reply, 0);
	if(!mb) {
		print("zero message\n");
		goto loop;
	}

	/*
	 * look for done guys
	 * also find spare slot->uop
	 */
busy:
	op = hp->out;
	uop = 0;
	for(i=0; i<NOUT; i++, op++) {
		if(op->mb == 0) {
			if(uop == 0)
				uop = op;
			continue;
		}
		if(*op->ip)
			continue;

		vmeflush(hp->vme->bus, op->hcmap, sizeof(Hcmd));
		vme2sysfree(hp->vme->bus, op->mbmap, op->mb->count);
		mbfree(op->mb);
		op->mb = 0;
		if(uop == 0)
			uop = op;
	}
	if(uop == 0) {
		if(hp->oubusy == 0)
			print("ou: busy!\n");
		hp->oubusy++;
		if(hp->oubusy > 1000000)
			hp->oubusy = 0;
		goto busy;
	}

	count = mb->count;
	uop->mb = mb;
	uop->mbmap = vme2sysmap(hp->vme->bus, (ulong)mb->data, count);
	wbackcache(mb->data, count);
	invalcache(mb->data, count);

	hc = uop->hc;
	hc->cmd = Ureply;
	hc->param[0] = mb->param;
	hc->param[1] = uop->mbmap;
	hc->param[2] = count;
	hc->param[3] = 0;
	if(SUM)
		hc->param[3] = addsum(mb->data, count);

	if(hp->verb) {
		print("cyo: cmd=reply u=%.8lux b=%.8lux c=%.8lux\n",
			hc->param[0], hc->param[1], hc->param[2]);
		prflush();
	}

	wbackcache(hc, sizeof(*hc));
	invalcache(hc, sizeof(*hc));

	qlock(&hp->issueq);
	i = hp->issuep;
	uop->ip = &com->reqstq[i];

	i++;
	if(i >= NRQ)
		i = 0;
	hp->issuep = i;
	qunlock(&hp->issueq);

	hp->count.count++;
	hp->rate.count += count;

	*uop->ip = uop->hcmap;

	goto loop;
}

/*
 * process pick up hotrod filled buffers
 */
int
cycmd(void *v)
{
	Cyclone *hp;

	hp = (Cyclone*)v;
	return hp->cmdwait;
}

void
cyclin(void)
{
	ulong u, s;
	int i, count;
	Chan *cp;
	Msgbuf *mb;
	Hhdr *hc;
	Hcmd *hcmd;
	Comm* com;
	Cyclone *hp;

	hp = getarg();
	print("cyi%d: start\n", hp-h);
	userinit(cyclou, hp, "cyo");

	cp = hp->chan;
	com = hp->vme->address;
	
loop:
	sleep(&hp->hrir, cycmd, hp);
	hp->cmdwait = 0;

l1:
	/*
	 * look at reply q from cyclone
	 */
	u = com->replyq[hp->replyp];
	if(u == 0)
		goto loop;
	com->replyq[hp->replyp] = 0;
	hp->replyp++;
	if(hp->replyp >= NRQ)
		hp->replyp = 0;

	hc = &hp->buf[hp->bhash[u%hp->vhash]];
	if(hc->hcmap == u)
		goto found;

	for(i=0; i<NBUF; i++) {
		hc = &hp->buf[i];
		if(hc->hcmap == u)
			goto found;
	}
	print("cyi: spurious %lux\n", u);
	goto l1;

found:
	vmeflush(hp->vme->bus, hc->hcmap, sizeof(Hcmd));
	invalcache(hc, sizeof(Hcmd));

	hcmd = hc->hcmd;
	if(hcmd->cmd != Ubuf) {
		print("cyi: cmd not Ubuf %d\n", hcmd->cmd);
		goto l1;
	}

	count = hc->mbsize;
	if(hcmd->param[3] > count) {
		print("cyi: rcount too big %d %lux\n", hcmd->param[3], &hcmd->param[3]);
		hcmd->param[3] = count;
	}
	mb = hc->mb;
	vme2sysfree(hp->vme->bus, hc->mbmap, count);
	invalcache(mb->data, count);

	mb->param = hcmd->param[0];	/* u */
	mb->count = hcmd->param[3];	/* actual count */
	if(hcmd->param[4]) {
		s = addsum(mb->data, mb->count);
		if(s != hcmd->param[4])
			print("cyi: checksum %lux sb %lux\n", s, hcmd->param[4]);
	}

	if(hp->verb) {
		print("cyi%d: cmd=buf u=%.8lux b=%.8lux c=%.8lux\n",
			hp-h, hcmd->param[0], hcmd->param[1], hcmd->param[3]);
		prflush();
	}

	send(cp->send, mb);

	hp->count.count++;
	hp->rate.count += mb->count;

	mb = mballoc(count, cp, Mbcycl2);
	hc->mbmap = vme2sysmap(hp->vme->bus, (ulong)mb->data, count);
	hc->mb = mb;

	hcmd->cmd = Ubuf;
	hcmd->param[0] = 0;			/* returned u */
	hcmd->param[1] = hc->mbmap;		/* message return */
	hcmd->param[2] = count;			/* size */
	hcmd->param[3] = 0;			/* returned count */
	hcmd->param[4] = 0;			/* returned cksum */

	wbackcache(mb->data, count);
	invalcache(mb->data, count);

	wbackcache(hcmd, sizeof(Hcmd));
	invalcache(hcmd, sizeof(Hcmd));

	qlock(&hp->issueq);
	i = hp->issuep;
	com->reqstq[i] = hc->hcmap;
	i++;
	if(i >= NRQ)
		i = 0;
	hp->issuep = i;
	qunlock(&hp->issueq);

	goto l1;
}

static void
dostart(Cyclone *hp)
{
	int i;
	Hhdr *hc;
	Comm *com;
	Hcmd *hcmd;
	struct { Hhdr hdr; Hcmd cmd; } cmd;

	print("cyclone start: %d\n", hp-h);

	cmd.cmd.cmd = Ureboot;
	if(cyclissue(hp, &cmd.hdr, &cmd.cmd, 1))
		print("	reboot didnt issue\n");

	hp->issuep = 0;		/* my pointer again */
	hp->replyp = 0;

	if(!hp->hpactive) {
		hp->hpactive = 1;
		userinit(cyclin, hp, "cyi");
	}
	fileinit(hp->chan);

	/* Set up the VME interrupt vector */
	com = hp->vme->address;
	com->vmevec = hp->vme->vector;

	for(i=0; i<NBUF; i++) {
		hc = &hp->buf[i];
		hcmd = hc->hcmd;

		hcmd->cmd = Ubuf;
		hcmd->param[0] = 0;			/* returned u */
		hcmd->param[1] = hc->mbmap;		/* message return */
		hcmd->param[2] = hc->mbsize;		/* size */
		hcmd->param[3] = 0;			/* returned count */

		wbackcache(hc->mb->data, hc->mbsize);
		invalcache(hc->mb->data, hc->mbsize);

		if(cyclissue(hp, hc, hcmd, 0)) {
			print("cyc%d: buf[%d] didnt issue\n", hp-h, i);
			return;
		}
	}
}

void
cyclstart(void)
{
	int i;

	for(i = 0; i < NCYC; i++)
		if(h[i].vme)
			dostart(&h[i]);
}

static
void
cmd_cycl(int argc, char *argv[])
{
	int i, c;
	Cyclone *hp;
	struct { Hhdr hdr; Hcmd cmd; } cmd;

	if(argc <= 1) {
		print("cycl [unit] reboot -- reboot\n");
		print("cycl [unit] intr -- fake an interrupt\n");
		print("cycl [unit] verbose -- chatty\n");
		print("cycl [unit] ping -- issue Uping\n");
		return;
	}

	hp = &h[0];
	i=1;
	if(argc >= 2 && argv[1][0] >= '0' && argv[1][0] <= '9') {
		c = number(argv[1], -1, 10);
		if(c < 0 || c >= NCYC) {
			print("bad cyclone\n");
			return;
		}
		hp = &h[c];
		i++;
	}
	if(hp->vme == 0) {
		print("cyc%d: not active\n", hp-h);
		return;
	}

	for(; i<argc; i++) {
		if(strcmp(argv[i], "reboot") == 0) {
			dostart(hp);
			continue;
		}
		if(strcmp(argv[i], "intr") == 0) {
			hp->cmdwait = 1;
			wakeup(&hp->hrir);
			continue;
		}
		if(strcmp(argv[i], "verbose") == 0) {
			hp->verb = !hp->verb;
			if(hp->verb)
				print("chatty\n");
			else
				print("quiet\n");
			continue;
		}
		if(strcmp(argv[i], "ping") == 0) {
			cmd.cmd.cmd = Uping;
			if(cyclissue(hp, &cmd.hdr, &cmd.cmd, 1))
				print("	ping didnt issue\n");
			continue;
		}
		print("unknown cycl subcommand\n");
	}
}

static
int
cyclissue(Cyclone *cy, Hhdr *hdr, Hcmd *cmd, int alloc)
{
	int i;
	long t;
	ulong *hp, m, map;
	Comm *com;

	map = hdr->hcmap;
	if(alloc)
		map = vme2sysmap(cy->vme->bus, (ulong)cmd, sizeof(Hcmd));
	wbackcache(cmd, sizeof(Hcmd));
	invalcache(cmd, sizeof(Hcmd));
	com = cy->vme->address;

	qlock(&cy->issueq);
	i = cy->issuep;
	hp = &com->reqstq[i];

	*hp = map;

	i++;
	if(i >= NRQ)
		i = 0;
	cy->issuep = i;
	qunlock(&cy->issueq);

	t = toytime() + SECOND(5);
	for(;;) {
		m = *hp;
		if(m == 0)
			break;
		if(toytime() >= t) {
			*hp = 0;
			break;
		}
	}

	if(alloc)
		vme2sysfree(cy->vme->bus, map, sizeof(Hcmd));
	else
		vmeflush(cy->vme->bus, map, sizeof(Hcmd));

	return m;
}

static
void
cmd_stats(int argc, char *argv[])
{
	int i;
	Vmedevice *vp;
	Cyclone *hp;
	

	USED(argc, argv);

	print("cyclone stats\n");
	hp = h;
	for(i = 0; i < NCYC; i++) {
		vp = hp->vme;
		if(vp) {
			print("	%s\n", vp->name);
			print("		work = %F mps\n", (Filta){&hp->count, 1});
			print("		rate = %F tBps\n", (Filta){&hp->rate, 1000});
		}
		hp++;
	}
}
