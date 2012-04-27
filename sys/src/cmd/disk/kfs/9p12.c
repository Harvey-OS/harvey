#include "all.h"

static int
readmsg(Chan *c, void *abuf, int n, int *ninep)
{
	int fd, len;
	uchar *buf;

	buf = abuf;
	fd = c->chan;
	qlock(&c->rlock);
	if(readn(fd, buf, 3) != 3){
		qunlock(&c->rlock);
		print("readn(3) fails: %r\n");
		return -1;
	}
	if((50 <= buf[0] && buf[0] <= 87 && (buf[0]&1)==0 && GBIT16(buf+1) == 0xFFFF)
	|| buf[0] == 86	/* Tattach */){
		*ninep = 1;
		/* assume message boundaries */
		n = read(fd, buf+3, n-3);
		if(n < 0){
			qunlock(&c->rlock);
			return -1;
		}
		return n+3;
	}

	*ninep = 2;
	if(read(fd, buf+3, 1) != 1){
		qunlock(&c->rlock);
		print("read(1) fails: %r\n");
		return -1;
	}
	len = GBIT32(buf);
	if(len > n){
		print("msg too large\n");
		qunlock(&c->rlock);
		return -1;
	}
	if(readn(fd, buf+4, len-4) != len-4){
		print("readn(%d) fails: %r\n", len-4);
		qunlock(&c->rlock);
		return -1;
	}
	qunlock(&c->rlock);
	return len;
}

int
startserveproc(void (*f)(Chan*, uchar*, int), char *name, Chan *c, uchar *b, int nb)
{
	int pid;

	switch(pid = rfork(RFMEM|RFPROC)){
	case -1:
		panic("can't fork");
	case 0:
		break;
	default:
		return pid;
	}
	procname = name;
	f(c, b, nb);
	_exits(nil);
	return -1;	/* can't happen */
}

void
serve(Chan *chan)
{
	int i, nin, p9, npid;
	uchar inbuf[1024];
	void (*s)(Chan*, uchar*, int);
	int *pid;
	Waitmsg *w;

	p9 = 0;
	if((nin = readmsg(chan, inbuf, sizeof inbuf, &p9)) < 0)
		return;

	switch(p9){
	default:
		print("unknown 9P type\n");
		return;
	case 1:
		s = serve9p1;
		break;
	case 2:
		s = serve9p2;
		break;
	}

	pid = malloc(sizeof(pid)*(conf.nserve-1));
	if(pid == nil)
		return;
	for(i=1; i<conf.nserve; i++)
		pid[i-1] = startserveproc(s, "srv", chan, nil, 0);

	(*s)(chan, inbuf, nin);

	/* wait till all other servers for this chan are done */
	for(npid = conf.nserve-1; npid > 0;){
		w = wait();
		if(w == 0)
			break;
		for(i = 0; i < conf.nserve-1; i++)
			if(pid[i] == w->pid)
				npid--;
		free(w);
	}
	free(pid);
}

