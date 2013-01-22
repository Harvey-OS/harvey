#include <u.h>
#include <libc.h>
#include "cec.h"

typedef struct {
	char	type;
	Pkt	p;
} Muxmsg;

#define	Muxo	(offsetof(Muxmsg, p))

typedef struct {
	int	fd;
	int	type;
	int	pid;
} Muxproc;

struct Mux {
	Muxmsg	m;
	Muxproc	p[2];
	int	pfd[2];
	int	inuse;
	int	to;
	int	retries;
	int	step;
	vlong	t0;
	vlong	t1;
};

static Mux smux = {
.inuse	= -1,
};

void
muxcec(int, int cfd)
{
	Muxmsg m;
	int l;

	m.type = Fcec;
	while((l = netget(&m.p, sizeof m.p)) > 0)
		if(write(cfd, &m, l+Muxo) != l+Muxo)
			break;
	exits("");
}

void
muxkbd(int kfd, int cfd)
{
	Muxmsg m;
	int o;

	m.type = Fkbd;
	o = offsetof(Muxmsg, p.data[0]);
	while((m.p.len = read(kfd, m.p.data, sizeof m.p.data)) > 0)
		if(write(cfd, &m, o+m.p.len) != o+m.p.len)
			break;
	m.type = Ffatal;
	write(cfd, &m, 4);
	exits("");
}

int
muxproc(Mux *m, Muxproc *p, int fd, void (*f)(int, int), int type)
{
	memset(p, 0, sizeof p);
	p->type = -1;
	switch(p->pid = rfork(RFPROC|RFFDG)){
	case -1:
		return -1;
	case 0:
		close(m->pfd[0]);
		f(fd, m->pfd[1]);
	default:
		p->fd = fd;
		p->type = type;
		return p->pid;
	}
}

void
muxfree(Mux *m)
{
	close(m->pfd[0]);
	close(m->pfd[1]);
	postnote(PNPROC, m->p[0].pid, "this note goes to 11");
	postnote(PNPROC, m->p[1].pid, "this note goes to 11");
	waitpid();
	waitpid();
	memset(m, 0, sizeof *m);
	m->inuse = -1;
}

Mux*
mux(int fd[2])
{
	Mux *m;

	if(smux.inuse != -1)
		sysfatal("mux in use");
	m = &smux;
	m->inuse = 1;
	if(pipe(m->pfd) == -1)
		sysfatal("pipe: %r");
	muxproc(m, m->p+0, fd[0], muxkbd, Fkbd);
	muxproc(m, m->p+1, fd[1], muxcec, Fcec);
	close(m->pfd[1]);
	return m;
}

#define MSEC()	(nsec()/1000000)

static void
timestamp(Mux *m)
{
	m->t0 = MSEC();
}

int
timeout(Mux *m)
{
	vlong t;

	if(m->to == 0)
		return -1;
	t = m->t0 + m->step;
	if(t > m->t1)
		t = m->t1;
	t -= MSEC();
	if(t < 5)
		t = 0;
	return t;
}

int
muxread(Mux *m, Pkt *p)
{
	int r, t;

	t = timeout(m);
	r = -1;
	if(t != 0){
		if(t > 0)
			alarm(t);
		r = read(m->pfd[0], &m->m, sizeof m->m);
		if(t > 0)
			alarm(0);
	}
	if(r == -1){
		timestamp(m);
		if(m->t0 + 5 >= m->t1)
			return Ftimedout;
		m->retries++;
		if(m->step < 1000)
			m->step <<= 1;
		return Ftimeout;
	}
	memcpy(p, &m->m.p, r-Muxo);
	return m->m.type;
}

void
muxtimeout(Mux *m, int to)
{
	m->retries = 0;
	timestamp(m);
	m->step = 75;
	m->to = to;
	m->t1 = m->t0 + to;
}
