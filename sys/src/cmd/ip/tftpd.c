#include <u.h>
#include <libc.h>
#include <bio.h>
#include <ip.h>
#include <ndb.h>

enum
{
	Maxpath=	128,
	Maxerr=		256,
};

int 	dbg;
int	restricted;
void	sendfile(int, char*, char*);
void	recvfile(int, char*, char*);
void	nak(int, int, char*);
void	ack(int, ushort);
void	clrcon(void);
void	setuser(void);
char*	sunkernel(char*);
void	remoteaddr(char*, char*, int);
void	doserve(int);

char	bigbuf[32768];
char	raddr[64];

char	*dir = "/lib/tftpd";
char	*dirsl;
int	dirsllen;
char	flog[] = "ipboot";
char	net[Maxpath];

enum
{
	Tftp_READ	= 1,
	Tftp_WRITE	= 2,
	Tftp_DATA	= 3,
	Tftp_ACK	= 4,
	Tftp_ERROR	= 5,
	Segsize		= 512,
};

void
usage(void)
{
	fprint(2, "usage: %s [-dr] [-h homedir] [-x netmtpt]\n", argv0);
	exits("usage");
}

void
main(int argc, char **argv)
{
	char buf[64];
	char adir[64], ldir[64];
	int cfd, lcfd, dfd;
	char *p;

	setnetmtpt(net, sizeof(net), nil);
	ARGBEGIN{
	case 'd':
		dbg++;
		break;
	case 'h':
		dir = ARGF();
		break;
	case 'r':
		restricted = 1;
		break;
	case 'x':
		p = ARGF();
		if(p == nil)
			usage();
		setnetmtpt(net, sizeof(net), p);
		break;
	default:
		usage();
	}ARGEND

	snprint(buf, sizeof buf, "%s/", dir);
	dirsl = strdup(buf);
	dirsllen = strlen(dirsl);

	fmtinstall('E', eipfmt);
	fmtinstall('I', eipfmt);

	if(chdir(dir) < 0)
		sysfatal("can't get to directory %s: %r", dir);

	if(!dbg)
		switch(rfork(RFNOTEG|RFPROC|RFFDG)) {
		case -1:
			sysfatal("fork: %r");
		case 0:
			break;
		default:
			exits(0);
		}

	syslog(dbg, flog, "started");

	sprint(buf, "%s/udp!*!69", net);
	cfd = announce(buf, adir);
	setuser();
	for(;;) {
		lcfd = listen(adir, ldir);
		if(lcfd < 0)
			sysfatal("listening: %r");

		switch(fork()) {
		case -1:
			sysfatal("fork: %r");
		case 0:
			dfd = accept(cfd, ldir);
			if(dfd < 0)
 				exits(0);
			remoteaddr(ldir, raddr, sizeof(raddr));
			doserve(dfd);
			exits("done");
			break;
		default:
			close(lcfd);
			continue;
		}
	}
}

void
doserve(int fd)
{
	int dlen;
	char *mode, *p;
	short op;

	dlen = read(fd, bigbuf, sizeof(bigbuf));
	if(dlen < 0)
		sysfatal("listen read: %r");

	op = (bigbuf[0]<<8) | bigbuf[1];
	dlen -= 2;
	mode = bigbuf+2;
	while(*mode != '\0' && dlen--)
		mode++;
	mode++;
	p = mode;
	while(*p && dlen--)
		p++;
	if(dlen == 0) {
		nak(fd, 0, "bad tftpmode");
		close(fd);
		syslog(dbg, flog, "bad mode %s", raddr);
		return;
	}

	if(op != Tftp_READ && op != Tftp_WRITE) {
		nak(fd, 4, "Illegal TFTP operation");
		close(fd);
		syslog(dbg, flog, "bad request %d %s", op, raddr);
		return;
	}

	if(restricted){
		if(strncmp(bigbuf+2, "../", 3) || strstr(bigbuf+2, "/../") ||
		  (bigbuf[2] == '/' && strncmp(bigbuf+2, dirsl, dirsllen)!=0)){
			nak(fd, 4, "Permission denied");
			close(fd);
			syslog(dbg, flog, "bad request %d %s", op, raddr);
			return;
		}
	}

	if(op == Tftp_READ)
		sendfile(fd, bigbuf+2, mode);
	else
		recvfile(fd, bigbuf+2, mode);
}

void
catcher(void *junk, char *msg)
{
	USED(junk);

	if(strncmp(msg, "exit", 4) == 0)
		noted(NDFLT);
	noted(NCONT);
}

void
sendfile(int fd, char *name, char *mode)
{
	int file;
	uchar buf[Segsize+4];
	uchar ack[1024];
	char errbuf[Maxerr];
	int ackblock, block, ret;
	int rexmit, n, al, txtry, rxl;
	short op;

	syslog(dbg, flog, "send file '%s' %s to %s", name, mode, raddr);
	name = sunkernel(name);
	if(name == 0){
		nak(fd, 0, "not in our database");
		return;
	}

	notify(catcher);

	file = open(name, OREAD);
	if(file < 0) {
		errstr(errbuf, sizeof errbuf);
		nak(fd, 0, errbuf);
		return;
	}
	block = 0;
	rexmit = 0;
	n = 0;
	for(txtry = 0; txtry < 5;) {
		if(rexmit == 0) {
			block++;
			buf[0] = 0;
			buf[1] = Tftp_DATA;
			buf[2] = block>>8;
			buf[3] = block;
			n = read(file, buf+4, Segsize);
			if(n < 0) {
				errstr(errbuf, sizeof errbuf);
				nak(fd, 0, errbuf);
				return;
			}
			txtry = 0;
		}
		else {
			syslog(dbg, flog, "rexmit %d %s:%d to %s", 4+n, name, block, raddr);
			txtry++;
		}

		ret = write(fd, buf, 4+n);
		if(ret < 0)
			sysfatal("tftpd: network write error: %r");

		for(rxl = 0; rxl < 10; rxl++) {
			rexmit = 0;
			alarm(500);
			al = read(fd, ack, sizeof(ack));
			alarm(0);
			if(al < 0) {
				rexmit = 1;
				break;
			}
			op = ack[0]<<8|ack[1];
			if(op == Tftp_ERROR)
				goto error;
			ackblock = ack[2]<<8|ack[3];
			if(ackblock == block)
				break;
			if(ackblock == 0xffff) {
				rexmit = 1;
				break;
			}
		}
		if(ret != Segsize+4 && rexmit == 0)
			break;
	}
error:	
	close(fd);
	close(file);
}

void
recvfile(int fd, char *name, char *mode)
{
	ushort op, block, inblock;
	uchar buf[Segsize+8];
	char errbuf[Maxerr];
	int n, ret, file;

	syslog(dbg, flog, "receive file '%s' %s from %s", name, mode, raddr);

	file = create(name, OWRITE, 0666);
	if(file < 0) {
		errstr(errbuf, sizeof errbuf);
		nak(fd, 0, errbuf);
		return;
	}

	block = 0;
	ack(fd, block);
	block++;

	for(;;) {
		alarm(15000);
		n = read(fd, buf, sizeof(buf));
		alarm(0);
		if(n < 0)
			goto error;
		op = buf[0]<<8|buf[1];
		if(op == Tftp_ERROR)
			goto error;

		n -= 4;
		inblock = buf[2]<<8|buf[3];
		if(op == Tftp_DATA) {
			if(inblock == block) {
				ret = write(file, buf, n);
				if(ret < 0) {
					errstr(errbuf, sizeof errbuf);
					nak(fd, 0, errbuf);
					goto error;
				}
				ack(fd, block);
				block++;
			}
			ack(fd, 0xffff);
		}
	}
error:
	close(file);
}

void
ack(int fd, ushort block)
{
	uchar ack[4];
	int n;

	ack[0] = 0;
	ack[1] = Tftp_ACK;
	ack[2] = block>>8;
	ack[3] = block;

	n = write(fd, ack, 4);
	if(n < 0)
		sysfatal("network write: %r");
}

void
nak(int fd, int code, char *msg)
{
	char buf[128];
	int n;

	buf[0] = 0;
	buf[1] = Tftp_ERROR;
	buf[2] = 0;
	buf[3] = code;
	strcpy(buf+4, msg);
	n = strlen(msg) + 4 + 1;
	n = write(fd, buf, n);
	if(n < 0)
		sysfatal("write nak: %r");
}

void
setuser(void)
{
	int f;

	f = open("/dev/user", OWRITE);
	if(f < 0)
		return;
	write(f, "none", sizeof("none"));
	close(f);
}

char*
lookup(char *sattr, char *sval, char *tattr, char *tval)
{
	static Ndb *db;
	char *attrs[1];
	Ndbtuple *t;

	if(db == nil)
		db = ndbopen(0);
	if(db == nil)
		return nil;

	if(sattr == nil)
		sattr = ipattr(sval);

	attrs[0] = tattr;
	t = ndbipinfo(db, sattr, sval, attrs, 1);
	if(t == nil)
		return nil;
	strcpy(tval, t->val);
	ndbfree(t);
	return tval;
}

/*
 *  for sun kernel boots, replace the requested file name with
 *  a one from our database.  If the database doesn't specify a file,
 *  don't answer.
 */
char*
sunkernel(char *name)
{
	ulong addr;
	uchar v4[IPv4addrlen];
	uchar v6[IPaddrlen];
	char buf[Ndbvlen];
	char ipbuf[Ndbvlen];

	if(strlen(name) != 14 || strncmp(name + 8, ".SUN", 4) != 0)
		return name;

	addr = strtoul(name, 0, 16);
	v4[0] = addr>>24;
	v4[1] = addr>>16;
	v4[2] = addr>>8;
	v4[3] = addr;
	v4tov6(v6, v4);
	sprint(ipbuf, "%I", v6);
	return lookup("ip", ipbuf, "bootf", buf);
}

void
remoteaddr(char *dir, char *raddr, int len)
{
	char buf[64];
	int fd, n;

	snprint(buf, sizeof(buf), "%s/remote", dir);
	fd = open(buf, OREAD);
	if(fd < 0){
		snprint(raddr, sizeof(raddr), "unknown");
		return;
	}
	n = read(fd, raddr, len-1);
	close(fd);
	if(n <= 0){
		snprint(raddr, sizeof(raddr), "unknown");
		return;
	}
	if(n > 0)
		n--;
	raddr[n] = 0;
}
