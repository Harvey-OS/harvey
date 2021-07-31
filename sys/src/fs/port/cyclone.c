#include	"all.h"
#include	"../cyc/comm.h"
#include	"io.h"

typedef	struct	Hcmd	Hcmd;
typedef	struct	Hhdr	Hhdr;
typedef	struct	Exec	Exec;
typedef	struct	Out	Out;


enum
{
	SUM	= 0,
	NBUF	= 100,
	NLBUF	= 50,
	NPARAM	= 6,
	NOUT	= 20,
	NHASH	= 290,
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

static
struct
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
} h;

static	int	cyclissue(Hhdr*, Hcmd*, int);
static	void	cmd_cycl(int, char*[]);
static	void	cmd_stats(int, char*[]);

void
cyclintr(Vmedevice *vp)
{

	USED(vp);
	h.cmdwait = 1;
	wakeup(&h.hrir);
}

int
cyclinit(Vmedevice *vp)
{
	int i, n, bv, bc, c;
	Msgbuf *m;
	Chan *cp;
	Hhdr *hc;

	print("cyclinit\n");

	/*
	 * does the board exist?
	com = vp->address;
	if(probe(&com->rom[0], sizeof(com->rom[0]))) {
		return -1;
	}
	 */

	memset(&h, 0, sizeof(h));
	h.vme = vp;
	h.reply = newqueue(10);
	cp = chaninit(Devcycl, 1);
	h.chan = cp;
	cp->send = serveq;
	cp->reply = h.reply;

	for(i=0; i<NBUF; i++) {
		n = LARGEBUF;
		if(i >= NLBUF)
			n = SMALLBUF;
		m = mballoc(n, h.chan, Mbcycl1);
		hc = &h.buf[i];
		hc->hcmd = ialloc(sizeof(Hcmd), LINESIZE);
		hc->mb = m;
		hc->mbsize = n;
		hc->mbmap = vme2sysmap(h.vme->bus, (ulong)m->data, n);
		hc->hcmap = vme2sysmap(h.vme->bus, (ulong)hc->hcmd, sizeof(Hcmd));
	}

	/*
	 * find best hash divisor
	 */
	bc = NHASH;
	bv = 0;
	for(h.vhash=10; h.vhash<=NHASH; h.vhash++) {
		memset(h.bhash, 0, sizeof(h.bhash));
		c = 0;
		for(i=0; i<NBUF; i++) {
			hc = &h.buf[i];
			n = hc->hcmap%h.vhash;
			if(h.bhash[n])
				c++;
			h.bhash[n] = 1;
		}
		if(c < bc) {
			bc = c;
			bv = h.vhash;
		}
	}
	print("best v = %d %d collisions\n", bv, bc);
	h.vhash = bv;
	memset(h.bhash, 0, sizeof(h.bhash));
	for(i=0; i<NBUF; i++) {
		hc = &h.buf[i];
		h.bhash[hc->hcmap%h.vhash] = i;
	}

	dofilter(&h.count);
	dofilter(&h.rate);

	cmd_install("cycl", "subcommand -- issue cyclone command", cmd_cycl);
	cmd_install("statc", "-- cyclone stats", cmd_stats);
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
	Out *op, *uop;
	Msgbuf *mb;
	Hcmd *hc;
	Comm *com;
	int i, count;

	for(i=0; i<NOUT; i++) {
		op = &h.out[i];
		op->mb = 0;
		op->hc = ialloc(sizeof(Hcmd), LINESIZE);
		op->hcmap = vme2sysmap(h.vme->bus, (ulong)op->hc, sizeof(Hcmd));
	}

	print("cyo: start\n");
	com = h.vme->address;

loop:
	mb = recv(h.reply, 0);
	if(!mb) {
		print("zero message\n");
		goto loop;
	}

	/*
	 * look for done guys
	 * also find spare slot->uop
	 */
busy:
	op = h.out;
	uop = 0;
	for(i=0; i<NOUT; i++, op++) {
		if(op->mb == 0) {
			if(uop == 0)
				uop = op;
			continue;
		}
		if(*op->ip)
			continue;

		vmeflush(h.vme->bus, op->hcmap, sizeof(Hcmd));
		vme2sysfree(h.vme->bus, op->mbmap, op->mb->count);
		mbfree(op->mb);
		op->mb = 0;
		if(uop == 0)
			uop = op;
	}
	if(uop == 0) {
		if(h.oubusy == 0)
			print("ou: busy!\n");
		h.oubusy++;
		if(h.oubusy > 1000000)
			h.oubusy = 0;
		goto busy;
	}

	count = mb->count;
	uop->mb = mb;
	uop->mbmap = vme2sysmap(h.vme->bus, (ulong)mb->data, count);
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

	if(h.verb) {
		print("cyo: cmd=reply u=%.8lux b=%.8lux c=%.8lux\n",
			hc->param[0], hc->param[1], hc->param[2]);
		prflush();
	}

	wbackcache(hc, sizeof(*hc));
	invalcache(hc, sizeof(*hc));

	qlock(&h.issueq);
	i = h.issuep;
	uop->ip = &com->reqstq[i];

	i++;
	if(i >= NRQ)
		i = 0;
	h.issuep = i;
	qunlock(&h.issueq);

	h.count.count++;
	h.rate.count += count;

	*uop->ip = uop->hcmap;

	goto loop;
}

/*
 * process pick up hotrod filled buffers
 */
int
cycmd(void *v)
{

	USED(v);
	return h.cmdwait;
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

	print("cyi: start\n");
	userinit(cyclou, 0, "cyo");
	cp = h.chan;
	com = h.vme->address;

loop:
	sleep(&h.hrir, cycmd, 0);
	h.cmdwait = 0;

l1:
	/*
	 * look at reply q from cyclone
	 */
	u = com->replyq[h.replyp];
	if(u == 0)
		goto loop;
	com->replyq[h.replyp] = 0;
	h.replyp++;
	if(h.replyp >= NRQ)
		h.replyp = 0;

	hc = &h.buf[h.bhash[u%h.vhash]];
	if(hc->hcmap == u)
		goto found;

	for(i=0; i<NBUF; i++) {
		hc = &h.buf[i];
		if(hc->hcmap == u)
			goto found;
	}
	print("hri: spurious %lux\n", u);
	goto l1;

found:
	vmeflush(h.vme->bus, hc->hcmap, sizeof(Hcmd));
	invalcache(hc, sizeof(Hcmd));

	hcmd = hc->hcmd;
	if(hcmd->cmd != Ubuf) {
		print("hri: cmd not Ubuf %d\n", hcmd->cmd);
		goto l1;
	}

	count = hc->mbsize;
	if(hcmd->param[3] > count) {
		print("hri: rcount too big %d %lux\n", hcmd->param[3], &hcmd->param[3]);
		hcmd->param[3] = count;
	}
	mb = hc->mb;
	vme2sysfree(h.vme->bus, hc->mbmap, count);
	invalcache(mb->data, count);

	mb->param = hcmd->param[0];	/* u */
	mb->count = hcmd->param[3];	/* actual count */
	if(hcmd->param[4]) {
		s = addsum(mb->data, mb->count);
		if(s != hcmd->param[4])
			print("hri: checksum %lux sb %lux\n", s, hcmd->param[4]);
	}

	if(h.verb) {
		print("cyi: cmd=buf u=%.8lux b=%.8lux c=%.8lux\n",
			hcmd->param[0], hcmd->param[1], hcmd->param[3]);
		prflush();
	}

	send(cp->send, mb);

	h.count.count++;
	h.rate.count += mb->count;

	mb = mballoc(count, cp, Mbcycl2);
	hc->mbmap = vme2sysmap(h.vme->bus, (ulong)mb->data, count);
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

	qlock(&h.issueq);
	i = h.issuep;
	com->reqstq[i] = hc->hcmap;
	i++;
	if(i >= NRQ)
		i = 0;
	h.issuep = i;
	qunlock(&h.issueq);

	goto l1;
}

void
cyclstart(void)
{
	int i;
	Hhdr *hc;
	Hcmd *hcmd;
	struct { Hhdr hdr; Hcmd cmd; } cmd;

	if(h.vme == 0) {
		print("no cyclone configured\n");
		return;
	}

	print("cyclone start\n");
	cmd.cmd.cmd = Ureboot;
	if(cyclissue(&cmd.hdr, &cmd.cmd, 1))
		print("	reboot didnt issue\n");

	h.issuep = 0;		/* my pointer again */
	h.replyp = 0;

	if(!h.hpactive) {
		h.hpactive = 1;
		userinit(cyclin, 0, "cyi");
	}
	fileinit(h.chan);

	for(i=0; i<NBUF; i++) {
		hc = &h.buf[i];
		hcmd = hc->hcmd;

		hcmd->cmd = Ubuf;
		hcmd->param[0] = 0;			/* returned u */
		hcmd->param[1] = hc->mbmap;		/* message return */
		hcmd->param[2] = hc->mbsize;		/* size */
		hcmd->param[3] = 0;			/* returned count */

		wbackcache(hc->mb->data, hc->mbsize);
		invalcache(hc->mb->data, hc->mbsize);

		if(cyclissue(hc, hcmd, 0)) {
			print("	buf[%d] didnt issue\n", i);
			return;
		}
	}
}

static
void
cmd_cycl(int argc, char *argv[])
{
	struct { Hhdr hdr; Hcmd cmd; } cmd;
	int i;

	if(argc <= 1) {
		print("cycl reboot -- reboot\n");
		print("cycl intr -- fake an interrupt\n");
		print("cycl verbose -- chatty\n");
		print("cycl ping -- issue Uping\n");
		return;
	}
	for(i=1; i<argc; i++) {
		if(strcmp(argv[i], "reboot") == 0) {
			cyclstart();
			continue;
		}
		if(strcmp(argv[i], "intr") == 0) {
			h.cmdwait = 1;
			wakeup(&h.hrir);
			continue;
		}
		if(strcmp(argv[i], "verbose") == 0) {
			h.verb = !h.verb;
			if(h.verb)
				print("chatty\n");
			else
				print("quiet\n");
			continue;
		}
		if(strcmp(argv[i], "ping") == 0) {
			cmd.cmd.cmd = Uping;
			if(cyclissue(&cmd.hdr, &cmd.cmd, 1))
				print("	ping didnt issue\n");
			continue;
		}
		print("unknown cycl subcommand\n");
	}
}

static
int
cyclissue(Hhdr *hdr, Hcmd *cmd, int alloc)
{
	int i;
	long t;
	ulong *hp, m, map;
	Comm *com;

	map = hdr->hcmap;
	if(alloc)
		map = vme2sysmap(h.vme->bus, (ulong)cmd, sizeof(Hcmd));
	wbackcache(cmd, sizeof(Hcmd));
	invalcache(cmd, sizeof(Hcmd));
	com = h.vme->address;

	qlock(&h.issueq);
	i = h.issuep;
	hp = &com->reqstq[i];
	*hp = map;

	i++;
	if(i >= NRQ)
		i = 0;
	h.issuep = i;
	qunlock(&h.issueq);

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
		vme2sysfree(h.vme->bus, map, sizeof(Hcmd));
	else
		vmeflush(h.vme->bus, map, sizeof(Hcmd));

	return m;
}

static
void
cmd_stats(int argc, char *argv[])
{
	Vmedevice *vp;

	USED(argc);
	USED(argv);

	print("cyclone stats\n");
	for(;;) {	/* loop for multipls cyclones */
		vp = h.vme;
		if(vp == 0)
			break;
		print("	%s\n", vp->name);
		print("		work = %F mps\n", (Filta){&h.count, 1});
		print("		rate = %F tBps\n", (Filta){&h.rate, 1000});
		break;
	}
}
