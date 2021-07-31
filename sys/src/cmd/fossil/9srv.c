#include "stdinc.h"

#include "9.h"

typedef struct Srv Srv;
typedef struct Srv {
	int	fd;
	int	srvfd;
	char*	service;
	char*	mntpnt;

	Srv*	next;
	Srv*	prev;
} Srv;

static struct {
	VtLock*	lock;

	Srv*	head;
	Srv*	tail;
} sbox;

static int
srvFd(char* name, int mode, int fd)
{
	int n, srvfd;
	char buf[10], srv[VtMaxStringSize];

	/*
	 * Drop a file descriptor with given name and mode into /srv.
	 * Create with ORCLOSE and don't close srvfd so it will be removed
	 * automatically on process exit.
	 */
	snprint(srv, sizeof(srv), "/srv/%s", name);
	if((srvfd = create(srv, ORCLOSE|OWRITE, mode)) < 0){
		snprint(srv, sizeof(srv), "#s/%s", name);
		if((srvfd = create(srv, ORCLOSE|OWRITE, mode)) < 0){
			vtSetError("create %s: %r", srv);
			return -1;
		}
	}

	n = snprint(buf, sizeof(buf), "%d", fd);
	if(write(srvfd, buf, n) < 0){
		close(srvfd);
		vtSetError("write %s: %r", srv);
		return -1;
	}

	return srvfd;
}

static void
srvFree(Srv* srv)
{
	if(srv->prev != nil)
		srv->prev->next = srv->next;
	else
		sbox.head = srv->next;
	if(srv->next != nil)
		srv->next->prev = srv->prev;
	else
		sbox.tail = srv->prev;

	if(srv->srvfd != -1)
		close(srv->srvfd);
	vtMemFree(srv->service);
	vtMemFree(srv->mntpnt);
	vtMemFree(srv);
}

static Srv*
srvAlloc(char* service, int mode, int fd)
{
	Srv *srv;
	int srvfd;

	vtLock(sbox.lock);
	for(srv = sbox.head; srv != nil; srv = srv->next){
		if(strcmp(srv->service, service) != 0)
			continue;
		vtSetError("srv: already serving '%s'", service);
		vtUnlock(sbox.lock);
		return nil;
	}

	if((srvfd = srvFd(service, mode, fd)) < 0){
		vtUnlock(sbox.lock);
		return nil;
	}
	close(fd);

	srv = vtMemAllocZ(sizeof(Srv));
	srv->srvfd = srvfd;
	srv->service = vtStrDup(service);

	if(sbox.tail != nil){
		srv->prev = sbox.tail;
		sbox.tail->next = srv;
	}
	else{
		sbox.head = srv;
		srv->prev = nil;
	}
	sbox.tail = srv;
	vtUnlock(sbox.lock);

	return srv;
}

static int
cmdSrv(int argc, char* argv[])
{
	Srv *srv;
	int dflag, fd[2], mode, pflag, r;
	char *usage = "usage: srv [-dp] [service]";

	dflag = pflag = 0;
	mode = 0666;

	ARGBEGIN{
	default:
		return cliError(usage);
	case 'd':
		dflag = 1;
		break;
	case 'p':
		pflag = 1;
		mode = 0600;
		break;
	}ARGEND

	switch(argc){
	default:
		return cliError(usage);
	case 0:
		vtRLock(sbox.lock);
		for(srv = sbox.head; srv != nil; srv = srv->next)
			consPrint("\t%s\t%d\n", srv->service, srv->srvfd);
		vtRUnlock(sbox.lock);

		return 1;
	case 1:
		if(!dflag)
			break;

		vtLock(sbox.lock);
		for(srv = sbox.head; srv != nil; srv = srv->next){
			if(strcmp(srv->service, argv[0]) != 0)
				continue;
			srvFree(srv);
			break;
		}
		vtUnlock(sbox.lock);

		if(srv == nil){
			vtSetError("srv: '%s' not found", argv[0]);
			return 0;
		}

		return 1;
	}

	if(pipe(fd) < 0){
		vtSetError("srv pipe: %r");
		return 0;
	}
	if((srv = srvAlloc(argv[0], mode, fd[0])) == nil){
		close(fd[0]); close(fd[1]);
		return 0;
	}

	if(pflag)
		r = consOpen(fd[1], srv->srvfd, -1);
	else
		r = (conAlloc(fd[1], argv[0]) != nil);
	if(r == 0){
		close(fd[1]);
		vtLock(sbox.lock);
		srvFree(srv);
		vtUnlock(sbox.lock);
	}

	return r;
}

int
srvInit(void)
{
	sbox.lock = vtLockAlloc();

	cliAddCmd("srv", cmdSrv);

	return 1;
}
