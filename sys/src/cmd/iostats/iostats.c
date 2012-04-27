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
	[Tversion]	Xversion,
	[Tauth]	Xauth,
	[Tflush]	Xflush,
	[Tattach]	Xattach,
	[Twalk]		Xwalk,
	[Topen]		slave,
	[Tcreate]	Xcreate,
	[Tclunk]	Xclunk,
	[Tread]		slave,
	[Twrite]	slave,
	[Tremove]	Xremove,
	[Tstat]		Xstat,
	[Twstat]	Xwstat,
};

int p[2];

void
usage(void)
{
	fprint(2, "usage: iostats [-d] [-f debugfile] cmds [args ...]\n");
	exits("usage");
}

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
	char buf[128];
	float brpsec, bwpsec, bppsec;
	int type, cpid, fspid, n;

	dbfile = DEBUGFILE;

	ARGBEGIN{
	case 'd':
		dbg++;
		break;
	case 'f':
		dbfile = ARGF();
		break;
	default:
		usage();
	}ARGEND

	if(argc == 0)
		usage();

	if(dbg) {
		close(2);
		create(dbfile, OWRITE, 0666);
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
		if(mount(p[0], -1, "/", MREPL, "") < 0)
			fatal("mount /");

		bind("#c/pid", "/dev/pid", MREPL);
		bind("#e", "/env", MREPL|MCREATE);
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
		while(cpid != waitpid())
			;
		postnote(PNPROC, fspid, DONESTR);
		while(fspid != waitpid())
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
	fhash = mallocz(sizeof(Fid*)*FHASHSIZE, 1);

	if(Workq == 0 || fhash == 0 || stats == 0)
		fatal("no initial memory");

	memset(Workq, 0, sizeof(Fsrpc)*Nr_workbufs);
	memset(stats, 0, sizeof(Stats));

	stats->rpc[Tversion].name = "version";
	stats->rpc[Tauth].name = "auth";
	stats->rpc[Tflush].name = "flush";
	stats->rpc[Tattach].name = "attach";
	stats->rpc[Twalk].name = "walk";
	stats->rpc[Topen].name = "open";
	stats->rpc[Tcreate].name = "create";
	stats->rpc[Tclunk].name = "clunk";
	stats->rpc[Tread].name = "read";
	stats->rpc[Twrite].name = "write";
	stats->rpc[Tremove].name = "remove";
	stats->rpc[Tstat].name = "stat";
	stats->rpc[Twstat].name = "wstat";

	for(n = 0; n < Maxrpc; n++)
		stats->rpc[n].lo = 10000000000LL;

	fmtinstall('M', dirmodefmt);
	fmtinstall('D', dirfmt);
	fmtinstall('F', fcallfmt);

	if(chdir("/") < 0)
		fatal("chdir");

	initroot();

	DEBUG(2, "statfs: %s\n", buf);

	notify(catcher);

	for(;;) {
		r = getsbuf();
		if(r == 0)
			fatal("Out of service buffers");

		n = read9pmsg(p[1], r->buf, sizeof(r->buf));
		if(done)
			break;
		if(n < 0)
			fatal("read server");

		if(convM2S(r->buf, n, &r->work) == 0)
			fatal("format error");

		stats->nrpc++;
		stats->nproto += n;

		DEBUG(2, "%F\n", &r->work);

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
	brpsec = (float)stats->totread / (((float)rpc->time/1e9)+.000001);

	rpc = &stats->rpc[Twrite];
	bwpsec = (float)stats->totwrite / (((float)rpc->time/1e9)+.000001);

	ttime = 0;
	for(n = 0; n < Maxrpc; n++) {
		rpc = &stats->rpc[n];
		if(rpc->count == 0)
			continue;
		ttime += rpc->time;
	}

	bppsec = (float)stats->nproto / ((ttime/1e9)+.000001);

	fprint(2, "\nread      %lud bytes, %g Kb/sec\n", stats->totread, brpsec/1024.0);
	fprint(2, "write     %lud bytes, %g Kb/sec\n", stats->totwrite, bwpsec/1024.0);
	fprint(2, "protocol  %lud bytes, %g Kb/sec\n", stats->nproto, bppsec/1024.0);
	fprint(2, "rpc       %lud count\n\n", stats->nrpc);

	fprint(2, "%-10s %5s %5s %5s %5s %5s          T       R\n", 
	      "Message", "Count", "Low", "High", "Time", "Averg");

	for(n = 0; n < Maxrpc; n++) {
		rpc = &stats->rpc[n];
		if(rpc->count == 0)
			continue;
		fprint(2, "%-10s %5lud %5llud %5llud %5llud %5llud ms %8lud %8lud bytes\n", 
			rpc->name, 
			rpc->count,
			rpc->lo/1000000,
			rpc->hi/1000000,
			rpc->time/1000000,
			rpc->time/1000000/rpc->count,
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

		fprint(2, "%5lud %8lud %8lud %8lud %8lud %s\n", fr->opens, fr->nread, fr->bread,
							fr->nwrite, fr->bwrite, s);
	}

	exits(0);
}

void
reply(Fcall *r, Fcall *t, char *err)
{
	uchar data[IOHDRSZ+Maxfdata];
	int n;

	t->tag = r->tag;
	t->fid = r->fid;
	if(err) {
		t->type = Rerror;
		t->ename = err;
	}
	else 
		t->type = r->type + 1;

	DEBUG(2, "\t%F\n", t);

	n = convS2M(t, data, sizeof data);
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
		fidfree = mallocz(sizeof(Fid) * Fidchunk, 1);
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
	Dir *dir;

	DEBUG(2, "\tfile: 0x%p %s name %s\n", parent, parent->name, name);

	for(f = parent->child; f; f = f->childlist)
		if(strcmp(name, f->name) == 0)
			break;

	if(f != nil && !f->inval)
		return f;
	makepath(buf, parent, name);
	dir = dirstat(buf);
	if(dir == nil)
		return 0;
	if(f != nil){
		free(dir);
		f->inval = 0;
		return f;
	}

	new = malloc(sizeof(File));
	if(new == 0)
		fatal("no memory");

	memset(new, 0, sizeof(File));
	new->name = strdup(name);
	if(new->name == nil)
		fatal("can't strdup");
	new->qid.type = dir->qid.type;
	new->qid.vers = dir->qid.vers;
	new->qid.path = ++qid;

	new->parent = parent;
	new->childlist = parent->child;
	parent->child = new;

	free(dir);
	return new;
}

void
initroot(void)
{
	Dir *dir;

	root = malloc(sizeof(File));
	if(root == 0)
		fatal("no memory");

	memset(root, 0, sizeof(File));
	root->name = strdup("/");
	if(root->name == nil)
		fatal("can't strdup");
	dir = dirstat(root->name);
	if(dir == nil)
		fatal("root stat");

	root->qid.type = dir->qid.type;
	root->qid.vers = dir->qid.vers;
	root->qid.path = ++qid;
	free(dir);
}

void
makepath(char *as, File *p, char *name)
{
	char *c, *seg[100];
	int i;
	char *s;

	seg[0] = name;
	for(i = 1; i < 100 && p; i++, p = p->parent){
		seg[i] = p->name;
		if(strcmp(p->name, "/") == 0)
			seg[i] = "";	/* will insert slash later */
	}

	s = as;
	while(i--) {
		for(c = seg[i]; *c; c++)
			*s++ = *c;
		*s++ = '/';
	}
	while(s[-1] == '/')
		s--;
	*s = '\0';
	if(as == s)	/* empty string is root */
		strcpy(as, "/");
}

void
fatal(char *s)
{
	Proc *m;

	fprint(2, "iostats: %s: %r\n", s);

	/* Clear away the slave children */
	for(m = Proclist; m; m = m->next)
		postnote(PNPROC, m->pid, "exit");

	exits("fatal");
}

char*
rdenv(char *v, char **end)
{
	int fd, n;
	char *buf;
	Dir *d;
	if((fd = open(v, OREAD)) == -1)
		return nil;
	d = dirfstat(fd);
	if(d == nil || (buf = malloc(d->length + 1)) == nil)
		return nil;
	n = (int)d->length;
	n = read(fd, buf, n);
	close(fd);
	if(n <= 0){
		free(buf);
		buf = nil;
	}else{
		if(buf[n-1] != '\0')
			buf[n++] = '\0';
		*end = &buf[n];
	}
	free(d);
	return buf;
}

char Defaultpath[] = ".\0/bin";
void
runprog(char *argv[])
{
	char *path, *ep, *p;
	char arg0[256];

	path = rdenv("/env/path", &ep);
	if(path == nil){
		path = Defaultpath;
		ep = path+sizeof(Defaultpath);
	}
	for(p = path; p < ep; p += strlen(p)+1){
		snprint(arg0, sizeof arg0, "%s/%s", p, argv[0]);
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

void
fidreport(Fid *f)
{
	char *p, path[128];
	Frec *fr;

	p = path;
	makepath(p, f->f, "");

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
