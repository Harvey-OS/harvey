#include <u.h>
#include <libc.h>
#include <bio.h>
#include <ctype.h>
#include <mach.h>
#define Extern extern
#include "acid.h"
#include "y.tab.h"

static void install(int);

void
nocore(void)
{
	int i;

	if(cormap == 0)
		return;

	for (i = 0; i < cormap->nsegs; i++)
		if (cormap->seg[i].inuse && cormap->seg[i].fd >= 0)
			close(cormap->seg[i].fd);
	free(cormap);
	cormap = 0;
}

void
sproc(int pid)
{
	Lsym *s;
	char buf[64];
	int i, fcor;

	if(symmap == 0)
		error("no map");

	snprint(buf, sizeof(buf), "/proc/%d/mem", pid);
	fcor = open(buf, ORDWR);
	if(fcor < 0)
		error("setproc: open %s: %r", buf);

	checkqid(symmap->seg[0].fd, pid);

	s = look("pid");
	s->v->ival = pid;

	nocore();
	cormap = attachproc(pid, kernel, fcor, &fhdr);
	if (cormap == 0)
		error("setproc: can't make coremap: %r");
	i = findseg(cormap, "text");
	if (i > 0)
		cormap->seg[i].name = "*text";
	i = findseg(cormap, "data");
	if (i > 0)
		cormap->seg[i].name = "*data";
	install(pid);
}

int
nproc(char **argv)
{
	char buf[128];
	int pid, i, fd;

	pid = fork();
	switch(pid) {
	case -1:
		error("new: fork %r");
	case 0:
		rfork(RFNAMEG|RFNOTEG);

		snprint(buf, sizeof(buf), "/proc/%d/ctl", getpid());
		fd = open(buf, ORDWR);
		if(fd < 0)
			fatal("new: open %s: %r", buf);
		write(fd, "hang", 4);
		close(fd);

		close(0);
		close(1);
		close(2);
		for(i = 3; i < NFD; i++)
			close(i);

		open("/dev/cons", OREAD);
		open("/dev/cons", OWRITE);
		open("/dev/cons", OWRITE);
		exec(argv[0], argv);
		fatal("new: exec %s: %r");
	default:
		install(pid);
		msg(pid, "waitstop");
		notes(pid);
		sproc(pid);
		dostop(pid);
		break;
	}

	return pid;
}

void
notes(int pid)
{
	Lsym *s;
	Value *v;
	int i, fd;
	char buf[128];
	List *l, **tail;

	s = look("notes");
	if(s == 0)
		return;
	v = s->v;

	snprint(buf, sizeof(buf), "/proc/%d/note", pid);
	fd = open(buf, OREAD);
	if(fd < 0)
		error("pid=%d: open note: %r", pid);

	v->set = 1;
	v->type = TLIST;
	v->l = 0;
	tail = &v->l;
	for(;;) {
		i = read(fd, buf, sizeof(buf));
		if(i <= 0)
			break;
		buf[i] = '\0';
		l = al(TSTRING);
		l->string = strnode(buf);
		l->fmt = 's';
		*tail = l;
		tail = &l->next;
	}
	close(fd);
}

void
dostop(int pid)
{
	Lsym *s;
	Node *np, *p;

	s = look("stopped");
	if(s && s->proc) {
		np = an(ONAME, ZN, ZN);
		np->sym = s;
		np->fmt = 'D';
		np->type = TINT;
		p = con(pid);
		p->fmt = 'D';
		np = an(OCALL, np, p);
		execute(np);
	}
}

static void
install(int pid)
{
	Lsym *s;
	List *l;
	char buf[128];
	int i, fd, new, p;

	new = -1;
	for(i = 0; i < Maxproc; i++) {
		p = ptab[i].pid;
		if(p == pid)
			return;
		if(p == 0 && new == -1)
			new = i;
	}
	if(new == -1)
		error("no free process slots");

	snprint(buf, sizeof(buf), "/proc/%d/ctl", pid);
	fd = open(buf, OWRITE);
	if(fd < 0)
		error("pid=%d: open ctl: %r", pid);
	ptab[new].pid = pid;
	ptab[new].ctl = fd;

	s = look("proclist");
	l = al(TINT);
	l->fmt = 'D';
	l->ival = pid;
	l->next = s->v->l;
	s->v->l = l;
	s->v->set = 1;
}

void
deinstall(int pid)
{
	int i;
	Lsym *s;
	List *f, **d;

	for(i = 0; i < Maxproc; i++) {
		if(ptab[i].pid == pid) {
			close(ptab[i].ctl);
			ptab[i].pid = 0;
			s = look("proclist");
			d = &s->v->l;
			for(f = *d; f; f = f->next) {
				if(f->ival == pid) {
					*d = f->next;
					break;
				}
			}
			s = look("pid");
			if(s->v->ival == pid)
				s->v->ival = 0;
			return;
		}
	}
}

void
msg(int pid, char *msg)
{
	int i;
	int l;
	char err[ERRMAX];

	for(i = 0; i < Maxproc; i++) {
		if(ptab[i].pid == pid) {
			l = strlen(msg);
			if(write(ptab[i].ctl, msg, l) != l) {
				errstr(err, sizeof err);
				if(strcmp(err, "process exited") == 0)
					deinstall(pid);
				error("msg: pid=%d %s: %s", pid, msg, err);
			}
			return;
		}
	}
	error("msg: pid=%d: not found for %s", pid, msg);
}

char *
getstatus(int pid)
{
	int fd, n;
	char *argv[16], buf[64];
	static char status[128];

	snprint(buf, sizeof(buf), "/proc/%d/status", pid);
	fd = open(buf, OREAD);
	if(fd < 0)
		error("open %s: %r", buf);

	n = read(fd, status, sizeof(status)-1);
	close(fd);
	if(n <= 0)
		error("read %s: %r", buf);
	status[n] = '\0';

	if(tokenize(status, argv, nelem(argv)-1) < 3)
		error("tokenize %s: %r", buf);

	return argv[2];
}

Waitmsg*
waitfor(int pid)
{
	Waitmsg *w;

	for(;;) {
		if((w = wait()) == nil)
			error("wait %r");
		if(w->pid == pid)
			return w;
		free(w);
	}
}
