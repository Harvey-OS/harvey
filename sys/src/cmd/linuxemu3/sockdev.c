#include <u.h>
#include <libc.h>
#include <ureg.h>
#include "dat.h"
#include "fns.h"
#include "linux.h"

typedef struct Socket Socket;
typedef struct Connectproc Connectproc;
typedef struct Listenproc Listenproc;

enum {
	Ctlsize = 128,
};

struct Socket
{
	Ufile;

	int				family;
	int				stype;
	int				protocol;

	int				other;
	char				net[40];
	char				name[Ctlsize];

	int				naddr;
	uchar			addr[40];

	void				*bufproc;
	Connectproc		*connectproc;
	Listenproc		*listenproc;

	int				connected;
	int				error;

	Socket			*next;
};

struct Connectproc
{
	Ref;
	QLock;
	Socket 	*sock;
	int		notefd;
	Uwaitq	wq;
	char		str[Ctlsize];
};

struct Listenproc
{
	Ref;
	QLock;
	Socket	*sock;
	int		notefd;
	Uwaitq	wq;
	Socket	*q;
	char		str[Ctlsize];
};

enum
{
	AF_UNIX			=1,
	AF_INET			=2,
	AF_INET6			=10,
};

enum
{
	SOCK_STREAM		=1,
	SOCK_DGRAM		=2,
	SOCK_RAW		=3,
};

static char*
srvname(char *npath, char *path, int len)
{
	char *p;

	p = strrchr(path, '/');
	if(p == 0)
		p = path;
	else
		p++;
	snprint(npath, len, "/srv/UD.%s", p);
	return npath;
}

static int
srvunixsock(int fd, char *path)
{
	int ret;
	int sfd;
	char buf[8+Ctlsize+1];

	sfd = -1;
	ret = -1;
	if(fd < 0)
		goto out;
	srvname(buf, path, sizeof(buf));
	remove(buf);
	if((sfd = create(buf, OWRITE, 0666)) < 0)
		goto out;
	sprint(buf, "%d", fd);
	if(write(sfd, buf, strlen(buf)) < 0)
		goto out;
	ret = 0;
out:
	if(sfd >= 0)
		close(sfd);
	return ret;
}

static void
unsrvunixsock(char *path)
{
	char buf[8+Ctlsize+1];

	srvname(buf, path, sizeof(buf));
	remove(buf);
}

static Socket*
allocsock(int family, int stype, int protocol)
{
	Socket *sock;

	sock = kmallocz(sizeof(*sock), 1);
	sock->family = family;
	sock->stype = stype;
	sock->protocol = protocol;
	sock->fd = -1;
	sock->other = -1;
	sock->ref = 1;
	sock->dev = SOCKDEV;
	sock->mode = O_RDWR;

	return sock;
}

static int
newsock(int family, int stype, int protocol)
{
	Socket *sock;
	char *net;
	char buf[Ctlsize];
	int pfd[2];
	int cfd, dfd;
	int n;
	int err;

	trace("newsock(%d, %d, %d)", family, stype, protocol);

	err = -EINVAL;
	switch(family){
	case AF_INET:
	case AF_INET6:
		switch(stype){
		case SOCK_DGRAM:
			net = "udp";
			break;
		case SOCK_STREAM:
			net = "tcp";
			break;
		default:
			trace("newsock() unknown socket type %d/%d", family, stype);
			return err;
		}
		break;
	case AF_UNIX:
		net = nil;
		break;

	default:
		trace("newsock() unknown network family %d", family);
		return err;
	}

	sock = allocsock(family, stype, protocol);
	cfd = -1;
	if(net == nil){
		if(pipe(pfd) < 0){
			err = mkerror();
			goto errout;
		}
		sock->other = pfd[1];
		sock->fd = pfd[0];
	} else {
		snprint(buf, sizeof(buf), "/net/%s/clone", net);
		if((cfd = open(buf, ORDWR)) < 0){
			err = mkerror();
			goto errout;
		}
		n = read(cfd, buf, sizeof(buf)-1);
		if(n < 0)
			err = mkerror();
		if(n <= 0)
			goto errout;
		buf[n] = 0;
		n = atoi(buf);
		snprint(buf, sizeof(buf), "/net/%s/%d/data", net, n);
		if((dfd = open(buf, ORDWR)) < 0){
			err = mkerror();
			goto errout;
		}
		close(cfd);
		sock->fd = dfd;
		snprint(sock->net, sizeof(sock->net), "/net/%s", net);
		snprint(sock->name, sizeof(sock->name), "%s/%d", sock->net, n);
	}
	return newfd(sock, FD_CLOEXEC);

errout:
	close(cfd);
	free(sock);
	return err;
}

static void
freeconnectproc(Connectproc *cp)
{
	if(cp == nil)
		return;
	qlock(cp);
	cp->sock = nil;
	if(decref(cp)){
		write(cp->notefd, "interrupt", 9);
		qunlock(cp);
		return;
	}
	qunlock(cp);
	close(cp->notefd);
	free(cp);
}

static void
freelistenproc(Listenproc *lp)
{
	Socket *q;

	if(lp == nil)
		return;
	qlock(lp);
	lp->sock = nil;
	if(decref(lp)){
		write(lp->notefd, "interrupt", 9);
		qunlock(lp);
		return;
	}
	while(q = lp->q){
		lp->q = q->next;
		putfile(q);
	}
	qunlock(lp);
	close(lp->notefd);
	free(lp);
}

static int
closesock(Ufile *file)
{
	Socket *sock = (Socket*)file;

	close(sock->fd);
	close(sock->other);
	freebufproc(sock->bufproc);
	freeconnectproc(sock->connectproc);
	freelistenproc(sock->listenproc);
	return 0;
}


static void
connectproc(void *aux)
{
	int fd, cfd, other;
	char buf[Ctlsize], tmp[8+Ctlsize+1];
	Connectproc *cp;
	Socket *sock;
	int err;

	cp =  (Connectproc*)aux;
	qlock(cp);
	if((sock = cp->sock) == nil)
		goto out;

	snprint(buf, sizeof(buf), "connectproc() %s", cp->str); 
	setprocname(buf);

	err = 0;
	switch(sock->family){
	case AF_UNIX:
		fd = sock->fd;
		other = sock->other;
		qunlock(cp);

		err = -ECONNREFUSED;
		srvname(tmp, cp->str, sizeof(buf));
		if((cfd = open(tmp, ORDWR)) < 0)
			break;

		memset(buf, 0, sizeof(buf));
		snprint(buf, sizeof(buf), "linuxemu.%d.%lux", getpid(), (ulong)sock);
		if(srvunixsock(other, buf) < 0){
			close(cfd);
			break;
		}

		/*
		 * write Ctrlsize-1 bytes so concurrent writes will not be merged together as
		 * Ctrlsize-1 is the size used in read(). see /sys/src/ape/lib/bsd/accept.c:87
		 * this should be fixed in ape's connect() as well.
		 */
		if(write(cfd, buf, sizeof(buf)-1) != sizeof(buf)-1){
			close(cfd);
			unsrvunixsock(buf);
			break;
		}
		close(cfd);
		if((read(fd, tmp, strlen(buf)) != strlen(buf)) || memcmp(buf, tmp, strlen(buf))){
			unsrvunixsock(buf);
			break;
		}
		unsrvunixsock(buf);
		err = 0;
		break;

	default:
		snprint(buf, sizeof(buf), "%s/ctl", sock->name);
		qunlock(cp);
		if((cfd = open(buf, ORDWR)) < 0){
			err = mkerror();
			break;
		}
		if(fprint(cfd, "connect %s", cp->str) < 0)
			err = mkerror();
		close(cfd);
	}

	qlock(cp);
	if((sock = cp->sock) == nil)
		goto out;
	if(err == 0){
		close(sock->other);
		sock->other = -1;
		sock->connected = 1;
	}
	sock->error = err;
out:
	wakeq(&cp->wq, MAXPROC);
	qunlock(cp);
	freeconnectproc(cp);
}

static int
sockaddr2str(Socket *sock, uchar *addr, int addrlen, char *buf, int nbuf)
{
	int err;

	err = -EINVAL;
	switch(sock->family){
	case AF_INET:
		if(addrlen < 8)
			break;
		err = snprint(buf, nbuf, "%d.%d.%d.%d!%d",
			(int)(addr[4]),
			(int)(addr[5]),
			(int)(addr[6]),
			(int)(addr[7]),
			(int)(((ulong)addr[2]<<8)|(ulong)addr[3]));
		break;

	case AF_INET6:
		/* TODO */
		break;

	case AF_UNIX:
		if(addrlen <= 2)
			break;
		addrlen -= 2;
		if(addrlen >= nbuf)
			addrlen = nbuf-1;
		memmove(buf, addr+2, addrlen);
		buf[addrlen] = 0;
		err = addrlen;
		break;
	}

	return err;
}

static int
connectsock(Socket *sock, uchar *addr, int addrlen)
{
	Connectproc *cp;
	int err;
	char buf[Ctlsize];
	int pid;

	if(sock->connected)
		return -EISCONN;
	if(sock->connectproc)
		return -EALREADY;

	if((err = sockaddr2str(sock, addr, addrlen, buf, sizeof(buf))) < 0)
		return err;

	cp = kmallocz(sizeof(*cp), 1);
	cp->ref = 2;
	cp->sock = sock;
	strncpy(cp->str, buf, sizeof(cp->str));

	qlock(cp);
	sock->error = 0;
	if((pid = procfork(connectproc, cp, 0)) < 0){
		qunlock(cp);
		free(cp);
		return mkerror();
	}
	snprint(buf, sizeof(buf), "/proc/%d/note", pid);
	cp->notefd = open(buf, OWRITE);

	if(addrlen > sizeof(sock->addr))
		addrlen = sizeof(sock->addr);
	sock->naddr = addrlen;
	memmove(sock->addr, addr, addrlen);

	sock->connectproc = cp;
	if(sock->mode & O_NONBLOCK){
		qunlock(cp);
		return -EINPROGRESS;
	}
	if((err = sleepq(&cp->wq, cp, 1)) == 0)
		err = sock->error;
	qunlock(cp);

	/*
	 * crazy shit is going on!
	 * see: http://www.madore.org/~david/computers/connect-intr.html
	*/
	if(err != -EINTR && err != -ERESTART){
		sock->connectproc = nil;
		freeconnectproc(cp);
	}
	return err;
}

static int
shutdownsock(Socket *sock, int how)
{
	USED(how);

	freebufproc(sock->bufproc);
	sock->bufproc = nil;
	freeconnectproc(sock->connectproc);
	sock->connectproc = nil;
	freelistenproc(sock->listenproc);
	sock->listenproc = nil;
	close(sock->fd);
	sock->fd = -1;
	sock->connected = 0;

	return 0;
}

static int 
bindsock(Socket *sock, uchar *addr, int addrlen)
{
	int port;
	int cfd;
	char buf[Ctlsize];

	port = -1;
	switch(sock->family){
	default:
		return -EINVAL;

	case AF_UNIX:
		break;
	case AF_INET:
		if(addrlen < 4)
			return -EINVAL;
		port = (int)(((ulong)addr[2]<<8)|(ulong)addr[3]);
		break;
	case AF_INET6:
		/* TODO */
		return -EINVAL;
	}

	if(port >= 0){
		snprint(buf, sizeof(buf), "%s/ctl", sock->name);
		if((cfd = open(buf, ORDWR)) < 0)
			return mkerror();
		if((fprint(cfd, "announce %d", port) < 0) || (fprint(cfd, "bind %d", port) < 0)){
			close(cfd);
			return mkerror();
		}
		close(cfd);
	}

	if(addrlen > sizeof(sock->addr))
		addrlen = sizeof(sock->addr);
	sock->naddr = addrlen;
	memmove(sock->addr, addr, addrlen);

	return 0;
}

static int
strtoip(char *str, uchar *ip, int iplen)
{
	int i, d, v6;
	char *p, *k;

	i = 0;
	v6 = 1;
	memset(ip, 0, iplen);
	for(p = str; *p; p++){
		if(*p == ':'){
			if(p[1] == ':'){
				p++;
				i = iplen;
				for(k = p+1; *k; k++){
					if(*k == ':'){
						v6 = 1;
						i -= 2;
					}
					if(*k == '.'){
						v6 = 0;
						i -= 1;
					}
				}
				i -= v6+1;
			} else {
				i += 2;
			}
			continue;
		} else if(*p == '.'){
			i++;
			continue;
		}

		for(k = p; *k && *k != '.' && *k != ':'; k++)
			;
		if(*k == '.'){
			v6 = 0;
		} else if(*k == ':'){
			v6 = 1;
		}

		if(i < 0 || i + v6+1 > iplen)
			return -1;

		if(*p >= '0' && *p <= '9'){
			d = *p - '0';
		} else if(v6 && (*p >= 'a' && *p <= 'f')){
			d = 0x0A + *p - 'a';
		} else if(v6 && (*p >= 'A' && *p <= 'F')){
			d = 0x0A + *p - 'A';
		} else {
			return -1;
		}

		if(v6){
			d |= ((int)ip[i]<<12 | (int)ip[i+1]<<4);
			ip[i] = (d>>8) & 0xFF;
			ip[i+1] = d & 0xFF;
		} else {
			ip[i] = ip[i]*10 + d;
		}
	}

	return i + v6+1;
}

static int
getsockaddr(Socket *sock, int remote, uchar *addr, int len)
{
	char buf[Ctlsize];
	char *p;
	uchar *a;
	int fd;
	int n, port;

	a = addr;
	switch(sock->family){
	case AF_UNIX:
		if(len < sock->naddr)
			break;
		memmove(a, sock->addr, sock->naddr);
		return sock->naddr;
	case AF_INET:
	case AF_INET6:
		snprint(buf, sizeof(buf), "%s/%s", sock->name, remote?"remote":"local");
		if((fd = open(buf, OREAD)) < 0)
			return mkerror();
		if((n = read(fd, buf, sizeof(buf)-1)) < 0){
			close(fd);
			return mkerror();
		}
		close(fd);
		if(n > 0 && buf[n-1] == '\n')
			n--;
		buf[n] = 0;
		break;
	default:
		return -EINVAL;
	}

	if((p = strrchr(buf, '!')) == nil)
		return -EINVAL;
	*p++ = 0;
	port = atoi(p);

	trace("getsockaddr(): ip=%s port=%d", buf, port);

	switch(sock->family){
	case AF_INET:
		if(len < 8)
			break;
		if(len > 16)
			len = 16;
		memset(a, 0, len);
		a[0] = sock->family & 0xFF;
		a[1] = (sock->family>>8) & 0xFF;
		a[2] = (port >> 8) & 0xFF;
		a[3] = port & 0xFF;
		if(strtoip(buf, &a[4], 4) < 0)
			break;
		return len;

	case AF_INET6:
		/* TODO */
		break;
	}

	return -EINVAL;
}

static void
listenproc(void *aux)
{
	Listenproc *lp;
	Socket *sock, *q;
	char buf[Ctlsize], tmp[8+Ctlsize+1];
	int cfd, fd, n;

	lp = (Listenproc*)aux;
	qlock(lp);
	if((sock = lp->sock) == nil)
		goto out;

	snprint(buf, sizeof(buf), "listenproc() %s", lp->str);
	setprocname(buf);

	for(;;){
		n = 0;
		cfd = -1;
		switch(sock->family){
		case AF_UNIX:
			srvunixsock(sock->other, lp->str);
			close(sock->other);
			sock->other = -1;
			fd = sock->fd;
			qunlock(lp);
			n = read(fd, buf, sizeof(buf)-1);
			qlock(lp);
			break;

		default:
			snprint(buf, sizeof(buf), "%s/listen", sock->name);
			qunlock(lp);
			if((cfd = open(buf, ORDWR)) >= 0)
				n = read(cfd, buf, sizeof(buf)-1);
			qlock(lp);
			if(n <= 0)
				close(cfd);
		}
		if(n <= 0)
			break;
		buf[n] = 0;

		if((sock = lp->sock) == nil){
			close(cfd);
			break;
		}

		switch(sock->family){
		case AF_UNIX:
			srvname(tmp, buf, sizeof(tmp));
			if((fd = open(tmp, ORDWR)) < 0)
				break;
			unsrvunixsock(buf);
			if(write(fd, buf, strlen(buf)) != strlen(buf)){
				close(fd);
				fd = -1;
			}
			buf[0] = 0;
			break;

		default:
			n = atoi(buf);
			snprint(buf, sizeof(buf), "%s/%d", sock->net, n);
			snprint(tmp, sizeof(tmp), "%s/data", buf);
			fd = open(tmp, ORDWR);
			close(cfd);
			break;
		}

		if(fd < 0)
			continue;

		q = allocsock(sock->family, sock->stype, sock->protocol);
		strncpy(q->net, sock->net, sizeof(q->net));
		strncpy(q->name, buf, sizeof(q->name));

		if(sock->family == AF_UNIX){
			memmove(q->addr, sock->addr, q->naddr = sock->naddr);
		} else {
			q->naddr = getsockaddr(q, 0, q->addr, sizeof(q->addr));
		}

		q->fd = fd;
		q->connected = 1;
		q->next = lp->q;
		lp->q = q;
		wakeq(&lp->wq, MAXPROC);
	}

	if(sock->family == AF_UNIX)
		unsrvunixsock(lp->str);
out:
	wakeq(&lp->wq, MAXPROC);
	qunlock(lp);
	freelistenproc(lp);
}


static int
listensock(Socket *sock)
{
	Listenproc *lp;
	int pid, err;
	char buf[Ctlsize];

	trace("listensock()");

	if(sock->listenproc)
		return 0;
	if((err = sockaddr2str(sock, sock->addr, sock->naddr, buf, sizeof(buf))) < 0)
		return err;

	lp = kmallocz(sizeof(*lp), 1);
	lp->ref = 2;
	lp->sock = sock;
	strncpy(lp->str, buf, sizeof(lp->str));

	qlock(lp);
	if((pid = procfork(listenproc, lp, 0)) < 0){
		qunlock(lp);
		free(lp);
		return mkerror();
	}
	snprint(buf, sizeof(buf), "/proc/%d/note", pid);
	lp->notefd = open(buf, OWRITE);
	sock->listenproc = lp;
	qunlock(lp);

	return 0;
}

static int
getsockname(Socket *sock, uchar *addr, int *paddrlen)
{
	int ret;

	trace("getsockname(%p, %p, %p (%x))", sock, addr, paddrlen, paddrlen ? *paddrlen : 0);

	if(addr == nil || paddrlen == nil)
		return -EINVAL;

	ret = sock->naddr;
	memmove(addr, sock->addr, ret);
	*paddrlen = ret;

	return ret;
}

static int
getpeername(Socket *sock, uchar *addr, int *paddrlen)
{
	int ret;

	trace("getpeername(%p, %p, %p (%x))", sock, addr, paddrlen, paddrlen ? *paddrlen : 0);

	if(addr == nil || paddrlen == nil)
		return -EINVAL;

	if((ret = getsockaddr(sock, 1, addr, *paddrlen)) > 0)
		*paddrlen = ret;
	return ret;
}

static int
acceptsock(Socket *sock, uchar *addr, int *paddrlen)
{
	Listenproc *lp;
	Socket *nsock;
	int err;

	trace("acceptsock(%p, %p, %p (%x))", sock, addr, paddrlen, paddrlen ? *paddrlen : 0);

	if((lp = sock->listenproc) == nil)
		return -EINVAL;

	qlock(lp);
	for(;;){
		if(nsock = lp->q){
			lp->q = nsock->next;
			nsock->next = nil;
			qunlock(lp);

			if(addr != nil && paddrlen != nil){
				err = getsockaddr(nsock, 1, addr, *paddrlen);
				*paddrlen = err < 0 ? 0 : err;
			}
			return newfd(nsock, FD_CLOEXEC);
		}

		if(sock->mode & O_NONBLOCK){
			err = -EAGAIN;
			break;
		}

		if((err = sleepq(&lp->wq, lp, 1)) < 0)
			break;
	}
	qunlock(lp);

	return err;
}

static int
socketpair(int family, int stype, int protocol, int sv[2])
{
	Socket *sock;
	int p[2];
	int i, fd;

	trace("socketpair(%d, %d, %d, %p)", family, stype, protocol, sv);

	if(family != AF_UNIX)
		return -EAFNOSUPPORT;
	if(pipe(p) < 0)
		return mkerror();
	for(i=0; i<2; i++){
		sock = allocsock(family, stype, protocol);
		sock->fd = p[i];
		sock->connected = 1;
		if((fd = newfd(sock, FD_CLOEXEC)) < 0){
			if(i > 0)
				sys_close(sv[0]);
			close(p[0]);
			close(p[1]);
			return fd;
		}
		sv[i] = fd;
	}
	return 0;
}

static void*
bufprocsock(Socket *sock)
{
	if(sock->bufproc == nil)
		sock->bufproc = newbufproc(sock->fd);
	return sock->bufproc;
}

static int
pollsock(Ufile *file, void *tab)
{
	Socket *sock = (Socket*)file;
	Listenproc *lp;
	Connectproc *cp;

	if(!sock->connected){
		if(lp = sock->listenproc){
			qlock(lp);
			pollwait(file, &lp->wq, tab);
			if(lp->q){
				qunlock(lp);
				return POLLIN;
			}
			qunlock(lp);
		}
		if(cp = sock->connectproc){
			qlock(cp);
			pollwait(file, &cp->wq, tab);
			if(sock->error < 0){
				qunlock(cp);
				return POLLOUT;
			}
			qunlock(cp);
		}
		return 0;
	}

	return pollbufproc(bufprocsock(sock), sock, tab);
}

static int
readsock(Ufile *file, void *buf, int len, vlong)
{
	Socket *sock = (Socket*)file;
	int ret;

	if(!sock->connected)
		return -ENOTCONN;
	if((sock->mode & O_NONBLOCK) || (sock->bufproc != nil)){
		ret = readbufproc(bufprocsock(sock), buf, len, 0, (sock->mode & O_NONBLOCK));
	} else {
		if(notifyme(1))
			return -ERESTART;
		ret = read(sock->fd, buf, len);
		notifyme(0);
		if(ret < 0)
			ret = mkerror();
	}
	return ret;
}

extern int pipewrite(int fd, void *buf, int len);

static int
writesock(Ufile *file, void *buf, int len, vlong)
{
	Socket *sock = (Socket*)file;
	int ret;

	if(!sock->connected)
		return -ENOTCONN;
	if(sock->family == AF_UNIX)
		return pipewrite(sock->fd, buf, len);
	if(notifyme(1))
		return -ERESTART;
	ret = write(sock->fd, buf, len);
	notifyme(0);
	if(ret < 0)
		ret = mkerror();
	return ret;
}

static int
ioctlsock(Ufile *file, int cmd, void *arg)
{
	Socket *sock = (Socket*)file;

	switch(cmd){
	default:
		return -ENOTTY;
	case 0x541B:
		{
			int r;

			if(arg == nil)
				return -EINVAL;
			if((r = nreadablebufproc(bufprocsock(sock))) < 0){
				*((int*)arg) = 0;
				return r;
			}
			*((int*)arg) = r;
		}
		return 0;
	}
}

static int
sendto(Socket *sock, void *data, int len, int, uchar *, int)
{
	trace("sendto(%p, %p, %d, ...)", sock, data, len);

	return writesock(sock, data, len, sock->off);
}

static int
recvfrom(Socket *sock, void *data, int len, int flags, uchar *addr, int addrlen)
{
	int ret;

	trace("recvfrom(%p, %p, %d, %x, %p, %d)", sock, data, len, flags, addr, addrlen);

	if(flags & 2){
		if(!sock->connected)
			return -ENOTCONN;
		ret = readbufproc(bufprocsock(sock), data, len, 1, 1);
	} else {
		ret = readsock(sock, data, len, sock->off);
	}
	if(addr){
		memmove(addr, sock->addr, sock->naddr);
	}
	return ret;
}

enum {
	SOL_SOCKET = 1,

	SO_DEBUG = 1,
	SO_REUSEADDR,
	SO_TYPE,
	SO_ERROR,
};

static int
getoptsock(Socket *sock, int lvl, int opt, char *ov, int *ol)
{
	trace("getoptsock(%p, %d, %d, %p, %p)", sock, lvl, opt, ov, ol);

	switch(lvl){
	default:
	Default:
		return -EINVAL;

	case SOL_SOCKET:
		switch(opt){
		default:
			goto Default;
		case SO_ERROR:
			*ol = sizeof(int);
			*((int*)ov) = sock->error;
			break;
		}
		break;
	}

	return 0;
}

enum {
	SYS_SOCKET=1,
	SYS_BIND,
	SYS_CONNECT,
	SYS_LISTEN,
	SYS_ACCEPT,
	SYS_GETSOCKNAME,
	SYS_GETPEERNAME,
	SYS_SOCKETPAIR,
	SYS_SEND,
	SYS_RECV,
	SYS_SENDTO,
	SYS_RECVFROM,
	SYS_SHUTDOWN,
	SYS_SETSOCKOPT,
	SYS_GETSOCKOPT,
	SYS_SENDMSG,
	SYS_RECVMSG,
};

int sys_linux_socketcall(int call, int *arg)
{
	Socket *sock;
	int ret;

	trace("sys_linux_socketcall(%d, %p)", call, arg);

	if(call == SYS_SOCKET)
		return newsock(arg[0], arg[1], arg[2]);

	if(call == SYS_SOCKETPAIR)
		return socketpair(arg[0], arg[1], arg[2], (int*)arg[3]);

	if((sock = (Socket*)fdgetfile(arg[0])) == nil)
		return -EBADF;

	if(sock->dev != SOCKDEV){
		putfile(sock);
		return -ENOTSOCK;
	}

	ret = -1;
	switch(call){
	case 	SYS_CONNECT:
		ret = connectsock(sock, (void*)arg[1], arg[2]);
		break;
	case 	SYS_SENDTO:
		ret = sendto(sock, (void*)arg[1], arg[2], arg[3], (void*)arg[4], arg[5]);
		break;
	case 	SYS_RECVFROM:
		ret = recvfrom(sock, (void*)arg[1], arg[2], arg[3], (void*)arg[4], arg[5]);
		break;
	case 	SYS_SEND:
		ret = sendto(sock, (void*)arg[1], arg[2], arg[3], nil, 0);
		break;
	case 	SYS_RECV:
		ret = recvfrom(sock, (void*)arg[1], arg[2], arg[3], nil, 0);
		break;
	case 	SYS_GETSOCKNAME:
		ret = getsockname(sock, (void*)arg[1], (void*)arg[2]);
		break;
	case 	SYS_GETPEERNAME:
		ret = getpeername(sock, (void*)arg[1], (void*)arg[2]);
		break;
	case 	SYS_SHUTDOWN:
		ret = shutdownsock(sock, arg[1]);
		break;
	case 	SYS_BIND:
		ret = bindsock(sock, (void*)arg[1], arg[2]);
		break;
	case 	SYS_LISTEN:
		ret = listensock(sock);
		break;
	case 	SYS_ACCEPT:
		ret = acceptsock(sock, (void*)arg[1], (void*)arg[2]);
		break;
	case 	SYS_SETSOCKOPT:
		ret = 0;
		break;
	case 	SYS_GETSOCKOPT:
		ret = getoptsock(sock, (int)arg[1], (int)arg[2], (char*)arg[3], (int*)arg[4]);
		break;
	case SYS_SENDMSG:
	case SYS_RECVMSG:
	default:
		trace("socketcall(): call %d not implemented", call);
	}

	putfile(sock);

	return ret;
}

static void
fillstat(Ustat *s)
{
	s->mode = 0666 | S_IFSOCK;
	s->uid = current->uid;
	s->gid = current->gid;
	s->size = 0;
}

static int
fstatsock(Ufile *, Ustat *s)
{
	fillstat(s);
	return 0;
};

static Udev sockdev = 
{
	.read = readsock,
	.write = writesock,
	.poll = pollsock,
	.close = closesock,
	.ioctl = ioctlsock,
	.fstat = fstatsock,
};

void sockdevinit(void)
{
	devtab[SOCKDEV] = &sockdev;
}
