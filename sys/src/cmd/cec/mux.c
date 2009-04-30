#include <u.h>
#include <libc.h>
#include "cec.h"

typedef struct {
	char	type;
	char	pad[3];
	Pkt	p;
} Muxmsg;

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
		if(write(cfd, &m, l+4) != l+4)
			break;
	exits("");
}

void
muxkbd(int kfd, int cfd)
{
	Muxmsg m;

	m.type = Fkbd;
	while((m.p.len = read(kfd, m.p.data, sizeof m.p.data)) > 0)
		if(write(cfd, &m, m.p.len+22) != m.p.len+22)
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

int
muxread(Mux *m, Pkt *p)
{
	if(read(m->pfd[0], &m->m, sizeof m->m) == -1)
		return -1;
	memcpy(p, &m->m.p, sizeof *p);
	return m->m.type;
}
