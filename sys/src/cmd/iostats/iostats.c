/*
 * iostats - Gather file system information
 */
#include <u.h>
#include <libc.h>
#include <auth.h>
#include <fcall.h>
#define Extern
#include "statfs.h"

void	runprog(char**);

void (*fcalls[])(Fsrpc*) =
{
	[Tnop]		Xnop,
	[Tsession]	Xsession,
	[Tflush]	Xflush,
	[Tattach]	Xattach,
	[Tclone]	Xclone,
	[Twalk]		Xwalk,
	[Topen]		slave,
	[Tcreate]	Xcreate,
	[Tclunk]	Xclunk,
	[Tread]		slave,
	[Twrite]	slave,
	[Tremove]	Xremove,
	[Tstat]		Xstat,
	[Twstat]	Xwstat,
	[Tclwalk]	Xclwalk,
};

int p[2];

void
main(int argc, char **argv)
{
	Fsrpc *r;
	Rpc *rpc;
	Proc *m;
	Frec *fr;
	Fid *fid;
	ulong ttime;
	char *dbfile, *s;
	char c, buf[128], err[ERRLEN];
	float brpsec, bwpsec, bppsec;
	int type, cpid, fspid, n, f;

	dbfile = DEBUGFILE;

	ARGBEGIN{
	case 'd':
		dbg++;
		break;
	case 'f':
		dbfile = ARGF();
		break;
	default:
	usage:
		fprint(2, "usage: iostats [-d] [-f debugfile] cmds [args ...]\n");
		exits("usage");
	}ARGEND

	if(argc == 0)
		goto usage;

	if(dbg) {
		close(2);
		create(dbfile, OWRITE|OTRUNC, 0666);
	}

	if(pipe(p) < 0)
		fatal("pipe");

	switch(cpid = fork()) {
	case -1:
		fatal("fork");
	case 0:
		close(p[1]);
		if(getwd(buf, sizeof(buf)) == 0)
			fatal("no working directory");

		rfork(RFENVG|RFNAMEG|RFNOTEG);
		if(mount(p[0], "/", MREPL, "") < 0)
			fatal("mount /");

		bind("#c/pid", "/dev/pid", MREPL);
		close(0);
		close(1);
		close(2);
		open("/fd/0", OREAD);
		open("/fd/1", OWRITE);
		open("/fd/2", OWRITE);

		if(chdir(buf) < 0)
			fatal("chdir");

		runprog(argv);
	default:
		close(p[0]);
	}

	switch(fspid = fork()) {
	default:
		while(cpid != wait(0))
			;
		postnote(PNPROC, fspid, DONESTR);
		while(fspid != wait(0))
			;
		exits(0);
	case -1:
		fatal("fork");
	case 0:
		break;
	}

	/* Allocate work queues in shared memory */
	malloc(Dsegpad);
	Workq = malloc(sizeof(Fsrpc)*Nr_workbufs);
	stats = malloc(sizeof(Stats));
	fhash = malloc(sizeof(Fid*)*FHASHSIZE);

	if(Workq == 0 || fhash == 0 || stats == 0)
		fatal("no initial memory");

	memset(Workq, 0, sizeof(Fsrpc)*Nr_workbufs);
	memset(stats, 0, sizeof(Stats));

	stats->rpc[Tnop].name = "nop";
	stats->rpc[Tsession].name = "session";
	stats->rpc[Tflush].name = "flush";
	stats->rpc[Tattach].name = "attach";
	stats->rpc[Tclone].name = "clone";
	stats->rpc[Twalk].name = "walk";
	stats->rpc[Topen].name = "open";
	stats->rpc[Tcreate].name = "create";
	stats->rpc[Tclunk].name = "clunk";
	stats->rpc[Tread].name = "read";
	stats->rpc[Twrite].name = "write";
	stats->rpc[Tremove].name = "remove";
	stats->rpc[Tstat].name = "stat";
	stats->rpc[Twstat].name = "wstat";
	stats->rpc[Tclwalk].name = "clwalk";

	for(n = 0; n < MAXRPC; n++)
		stats->rpc[n].loms = 10000000;

	fmtinstall('F', fcallconv);

	if(chdir("/") < 0)
		fatal("chdir");

	initroot();

	DEBUG(2, "statfs: %s\n", buf);

	notify(catcher);

	for(;;) {
		r = getsbuf();
		if(r == 0)
			fatal("Out of service buffers");

		n = read(p[1], r->buf, sizeof(r->buf));
		if(done)
			break;
		if(n < 0)
			fatal("read server");

		if(convM2S(r->buf, &r->work, n) == 0)
			fatal("format error");

		stats->nrpc++;
		stats->nproto += n;

		if(r->work.fid < 0)
			fatal("fid out of range");

		DEBUG(2, "%F\n", &r->work, &r->work);

		type = r->work.type;
		rpc = &stats->rpc[type];
		rpc->count++;
		rpc->bin += n;
		(fcalls[type])(r);
	}

	/* Clear away the slave children */
	for(m = Proclist; m; m = m->next)
		postnote(PNPROC, m->pid, "kill");

	rpc = &stats->rpc[Tread];
	brpsec = (float)stats->totread / (((float)rpc->time/1000.0)+.000001);

	rpc = &stats->rpc[Twrite];
	bwpsec = (float)stats->totwrite / (((float)rpc->time/1000.0)+.000001);

	ttime = 0;
	for(n = 0; n < MAXRPC; n++) {
		rpc = &stats->rpc[n];
		if(rpc->count == 0)
			continue;
		ttime += rpc->time;
	}

	bppsec = (float)stats->nproto / ((ttime/1000.0)+.000001);

	fprint(2, "\nread      %d bytes, %g Kb/sec\n", stats->totread, brpsec/1024.0);
	fprint(2, "write     %d bytes, %g Kb/sec\n", stats->totwrite, bwpsec/1024.0);
	fprint(2, "protocol  %d bytes, %g Kb/sec\n", stats->nproto, bppsec/1024.0);
	fprint(2, "rpc       %d count\n\n", stats->nrpc);

	fprint(2, "%-10s %5s %5s %5s %5s %5s          in      out\n", 
	      "Message", "Count", "Low", "High", "Time", "Averg");

	for(n = 0; n < MAXRPC; n++) {
		rpc = &stats->rpc[n];
		if(rpc->count == 0)
			continue;
		fprint(2, "%-10s %5d %5d %5d %5d %5d ms %8d %8d bytes\n", 
			rpc->name, 
			rpc->count,
			rpc->loms,
			rpc->hims,
			rpc->time,
			rpc->time/rpc->count,
			rpc->bin,
			rpc->bout);
	}

	for(n = 0; n < FHASHSIZE; n++)
		for(fid = fhash[n]; fid; fid = fid->next)
			if(fid->nread || fid->nwrite)
				fidreport(fid);
	if(frhead == 0)
		exits(0);

	fprint(2, "\nOpens    Reads  (bytes)   Writes  (bytes) File\n");
	for(fr = frhead; fr; fr = fr->next) {
		s = fr->op;
		if(*s) {
			if(strcmp(s, "/fd/0") == 0)
				s = "(stdin)";
			else
			if(strcmp(s, "/fd/1") == 0)
				s = "(stdout)";
			else
			if(strcmp(s, "/fd/2") == 0)
				s = "(stderr)";
		}
		else
			s = "/.";

		fprint(2, "%5d %8d %8d %8d %8d %s\n", fr->opens, fr->nread, fr->bread,
							fr->nwrite, fr->bwrite, s);
	}

	exits(0);
}

void
reply(Fcall *r, Fcall *t, char *err)
{
	char data[MAXFDATA+MAXMSG];
	int n;

	t->tag = r->tag;
	t->fid = r->fid;
	if(err) {
		t->type = Rerror;
		strncpy(t->ename, err, ERRLEN);
	}
	else 
		t->type = r->type + 1;

	DEBUG(2, "\t%F\n", t);

	n = convS2M(t, data);
	if(write(p[1], data, n)!=n)
		fatal("mount write");
	stats->nproto += n;
	stats->rpc[t->type-1].bout += n;
}

Fid *
getfid(int nr)
{
	Fid *f;

	for(f = fidhash(nr); f; f = f->next)
		if(f->nr == nr)
			return f;

	return 0;
}

int
freefid(int nr)
{
	Fid *f, **l;

	l = &fidhash(nr);
	for(f = *l; f; f = f->next) {
		if(f->nr == nr) {
			*l = f->next;
			f->next = fidfree;
			fidfree = f;
			return 1;
		}
		l = &f->next;
	}

	return 0;	
}

Fid *
newfid(int nr)
{
	Fid *new, **l;
	int i;

	l = &fidhash(nr);
	for(new = *l; new; new = new->next)
		if(new->nr == nr)
			return 0;

	if(fidfree == 0) {
		fidfree = malloc(sizeof(Fid) * Fidchunk);
		if(fidfree == 0)
			fatal("out of memory");

		for(i = 0; i < Fidchunk-1; i++)
			fidfree[i].next = &fidfree[i+1];

		fidfree[Fidchunk-1].next = 0;
	}

	new = fidfree;
	fidfree = new->next;

	memset(new, 0, sizeof(Fid));
	new->next = *l;
	*l = new;
	new->nr = nr;
	new->fid = -1;
	new->nread = 0;
	new->nwrite = 0;
	new->bread = 0;
	new->bwrite = 0;

	return new;	
}

Fsrpc *
getsbuf(void)
{
	static int ap;
	int look;
	Fsrpc *wb;

	for(look = 0; look < Nr_workbufs; look++) {
		if(++ap == Nr_workbufs)
			ap = 0;
		if(Workq[ap].busy == 0)
			break;
	}

	if(look == Nr_workbufs)
		fatal("No more work buffers");

	wb = &Workq[ap];
	wb->pid = 0;
	wb->canint = 0;
	wb->flushtag = NOTAG;
	wb->busy = 1;

	return wb;
}

char *
strcatalloc(char *p, char *n)
{
	char *v;

	v = realloc(p, strlen(p)+strlen(n)+1);
	if(v == 0)
		fatal("no memory");
	strcat(v, n);
	return v;
}

File *
file(File *parent, char *name)
{
	char buf[128];
	File *f, *new;
	Dir dir;

	DEBUG(2, "\tfile: 0x%x %s name %s\n", parent, parent->name, name);

	for(f = parent->child; f; f = f->childlist)
		if(strcmp(name, f->name) == 0)
			return f;

	makepath(buf, parent, name);
	if(dirstat(buf, &dir) < 0)
		return 0;
	
	new = malloc(sizeof(File));
	if(new == 0)
		fatal("no memory");

	memset(new, 0, sizeof(File));
	strcpy(new->name, name);
	new->qid.vers = dir.qid.vers;
	new->qid.path = (dir.qid.path&CHDIR)|++qid;

	new->parent = parent;
	new->childlist = parent->child;
	parent->child = new;

	return new;
}

void
initroot(void)
{
	Dir dir;

	root = malloc(sizeof(File));
	if(root == 0)
		fatal("no memory");

	memset(root, 0, sizeof(File));
	strcpy(root->name, ".");
	if(dirstat(root->name, &dir) < 0)
		fatal("root stat");

	root->qid.vers = dir.qid.vers;
	root->qid.path = (dir.qid.path&CHDIR)|++qid;
}

void
makepath(char *s, File *p, char *name)
{
	char *c, *seg[100];
	int i;

	seg[0] = name;
	for(i = 1; i < 100 && p; i++, p = p->parent)
		seg[i] = p->name;

	while(i--) {
		for(c = seg[i]; *c; c++)
			*s++ = *c;
		*s++ = '/';
	}
	while(s[-1] == '/')
		s--;
	*s = '\0';
}

void
fatal(char *s)
{
	char buf[128], err[ERRLEN];
	Proc *m;

	fprint(2, "iostats: %s: %r\n", s);

	/* Clear away the slave children */
	for(m = Proclist; m; m = m->next)
		postnote(PNPROC, m->pid, "exit");

	exits("fatal");
}

void
runprog(char *argv[])
{
	char arg0[128];

	exec(argv[0], argv);
	if (*argv[0] != '/' && strncmp(argv[0],"./",2) != 0) {
		sprint(arg0, "/bin/%s", argv[0]);
		exec(arg0, argv);
	}
	fatal("exec");
}

void
catcher(void *a, char *msg)
{
	USED(a);
	if(strcmp(msg, DONESTR) == 0) {
		done = 1;
		noted(NCONT);
	}
	if(strcmp(msg, "exit") == 0)
		exits("exit");

	noted(NDFLT);
}

ulong
msec(void)
{
	char b[20];
	int n;
	static int f = -1;

	if(f < 0)
		f = open("#c/msec", OREAD|OCEXEC);
	if(f >= 0) {
		do {
			seek(f, 0, 0);
			n = read(f, b, sizeof(b));
		}while(n != 12 && n >= 0);
		b[n] = 0;
	}
	return atol(b);
}

void
fidreport(Fid *f)
{
	char *p, path[128], buf[1024];
	Frec *fr;

	p = path;
	makepath(p, f->f, "");
	p++;

	for(fr = frhead; fr; fr = fr->next) {
		if(strcmp(fr->op, p) == 0) {
			fr->nread += f->nread;
			fr->nwrite += f->nwrite;
			fr->bread += f->bread;
			fr->bwrite += f->bwrite;
			fr->opens++;
			return;
		}
	}

	fr = malloc(sizeof(Frec));
	if(fr == 0 || (fr->op = strdup(p)) == 0)
		fatal("no memory");

	fr->nread = f->nread;
	fr->nwrite = f->nwrite;
	fr->bread = f->bread;
	fr->bwrite = f->bwrite;
	fr->opens = 1;
	if(frhead == 0) {
		frhead = fr;
		frtail = fr;
	}
	else {
		frtail->next = fr;
		frtail = fr;
	}
	fr->next = 0;
}
