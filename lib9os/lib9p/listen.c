#include <u.h>
#include <libc.h>
#include <auth.h>
#include <fcall.h>
#include <thread.h>
#include <9p.h>

static void listenproc(void*);
static void srvproc(void*);
static char *getremotesys(char*);

void
_listensrv(Srv *os, char *addr)
{
	Srv *s;

	if(_forker == nil)
		sysfatal("no forker");
	s = emalloc9p(sizeof *s);
	*s = *os;
	s->addr = estrdup9p(addr);
	_forker(listenproc, s, 0);
}

static void
listenproc(void *v)
{
	char ndir[NETPATHLEN], dir[NETPATHLEN];
	int ctl, data, nctl;
	Srv *os, *s;
	
	os = v;
	ctl = announce(os->addr, dir);
	if(ctl < 0){
		fprint(2, "%s: announce %s: %r", argv0, os->addr);
		return;
	}

	for(;;){
		nctl = listen(dir, ndir);
		if(nctl < 0){
			fprint(2, "%s: listen %s: %r", argv0, os->addr);
			break;
		}
		
		data = accept(ctl, ndir);
		if(data < 0){
			fprint(2, "%s: accept %s: %r\n", argv0, ndir);
			continue;
		}

		s = emalloc9p(sizeof *s);
		*s = *os;
		s->addr = getremotesys(ndir);
		s->infd = s->outfd = data;
		s->fpool = nil;
		s->rpool = nil;
		s->rbuf = nil;
		s->wbuf = nil;
		_forker(srvproc, s, 0);
	}
	free(os->addr);
	free(os);
}

static void
srvproc(void *v)
{
	int data;
	Srv *s;
	
	s = v;
	data = s->infd;
	srv(s);
	close(data);
	free(s->addr);
	free(s);
}

static char*
getremotesys(char *ndir)
{
	char buf[128], *serv, *sys;
	int fd, n;

	snprint(buf, sizeof buf, "%s/remote", ndir);
	sys = nil;
	fd = open(buf, OREAD);
	if(fd >= 0){
		n = read(fd, buf, sizeof(buf)-1);
		if(n>0){
			buf[n-1] = 0;
			serv = strchr(buf, '!');
			if(serv)
				*serv = 0;
			sys = estrdup9p(buf);
		}
		close(fd);
	}
	if(sys == nil)
		sys = estrdup9p("unknown");
	return sys;
}
