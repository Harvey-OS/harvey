#include <u.h>
#include <libc.h>
#include <bio.h>
#include <ip.h>
#include <ndb.h>

void	fatal(int syserr, char *fmt, ...);
void	openlisten(void);

int 	dbg;
int	restricted;
int	tftpreq;
int	tftpaddr;
int	tftpctl;
void	openlisten(void);
void	sendfile(int, char*, char*);
void	recvfile(int, char*, char*);
void	nak(int, int, char*);
void	ack(int, ushort);
void	clrcon(void);
void	setuser(void);
char*	sunkernel(char*);

char	mbuf[32768];
char	raddr[32];

char	*dir = "/lib/tftpd";
char	*dirsl;
int	dirsllen;
char	flog[] = "ipboot";

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
main(int argc, char **argv)
{
	int n, dlen, clen;
	char connect[64], buf[64], datadir[64];
	char *mode, *p;
	short op;
	int ctl, data;

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
	default:
		fprint(2, "usage: tftpd [-dr] [-h homedir]\n");
		exits("usage");
	}ARGEND
	USED(argc); USED(argv);

	snprint(buf, sizeof buf, "%s/", dir);
	dirsl = strdup(buf);
	dirsllen = strlen(dirsl);

	fmtinstall('E', eipconv);
	fmtinstall('I', eipconv);

	if(chdir(dir) < 0)
		fatal(1, "cant get to directory %s", dir);

	switch(rfork(RFNOTEG|RFPROC|RFFDG)) {
	case -1:
		fatal(1, "fork");
	case 0:
		break;
	default:
		exits(0);
	}

	syslog(dbg, flog, "started");

	openlisten();
	setuser();
	for(;;) {
		dlen = read(tftpreq, mbuf, sizeof(mbuf));
		if(dlen < 0)
			fatal(1, "listen read");
		seek(tftpaddr, 0, 0);
		clen = read(tftpaddr, raddr, sizeof(raddr));
		if(clen < 0)
			fatal(1, "request address read");
		raddr[clen-1] = '\0';
		clrcon();

		ctl = open("/net/udp/clone", ORDWR);
		if(ctl < 0)
			fatal(1, "open udp clone");
		n = read(ctl, buf, sizeof(buf));
		if(n < 0)
			fatal(1, "read udp ctl");
		buf[n] = 0;

		clen = sprint(connect, "connect %s", raddr);
		n = write(ctl, connect, clen);
		if(n < 0)
			fatal(1, "udp %s", raddr);

		sprint(datadir, "/net/udp/%s/data", buf);
		data = open(datadir, ORDWR);
		if(data < 0)
			fatal(1, "open udp data");

		close(ctl);

		dlen -= 2;
		mode = mbuf+2;
		while(*mode != '\0' && dlen--)
			mode++;
		mode++;
		p = mode;
		while(*p && dlen--)
			p++;
		if(dlen == 0) {
			nak(data, 0, "bad tftpmode");
			close(data);
			syslog(dbg, flog, "bad mode %s", raddr);
			continue;
		}

		op = mbuf[0]<<8 | mbuf[1];
		if(op != Tftp_READ && op != Tftp_WRITE) {
			nak(data, 4, "Illegal TFTP operation");
			close(data);
			syslog(dbg, flog, "bad request %d %s", op, raddr);
			continue;
		}
		if(restricted){
			if(strncmp(mbuf+2, "../", 3) || strstr(mbuf+2, "/../") ||
			  (mbuf[2] == '/' && strncmp(mbuf+2, dirsl, dirsllen)!=0)){
				nak(data, 4, "Permission denied");
				close(data);
				syslog(dbg, flog, "bad request %d %s", op, raddr);
				continue;
			}
		}
		switch(fork()) {
		case -1:
			fatal(1, "fork");
		case 0:
			if(op == Tftp_READ)
				sendfile(data, mbuf+2, mode);
			else
				recvfile(data, mbuf+2, mode);
			exits("done");
		default:
			close(data);
		}
	}	
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
	char errbuf[ERRLEN];
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
		errstr(errbuf);
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
				errstr(errbuf);
				nak(fd, 0, errbuf);
				return;
			}
			txtry = 0;
		}
		else
			txtry++;
		ret = write(fd, buf, 4+n);
		if(ret < 0)
			fatal(1, "tftp: network write error");

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
	char errbuf[ERRLEN];
	int n, ret, file;

	syslog(dbg, flog, "receive file '%s' %s from %s", name, mode, raddr);

	file = create(name, OWRITE, 0666);
	if(file < 0) {
		errstr(errbuf);
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
					errstr(errbuf);
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
		fatal(1, "network write");
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
		fatal(1, "write nak");
}

void
fatal(int syserr, char *fmt, ...)
{
	char buf[ERRLEN], sysbuf[ERRLEN];

	doprint(buf, buf+sizeof(buf), fmt, (&fmt+1));
	if(syserr) {
		errstr(sysbuf);
		fprint(2, "tftpd: %s: %s\n", buf, sysbuf);
	}
	else
		fprint(2, "tftpd: %s\n", buf);
	exits(buf);
}

void
openlisten(void)
{
	char buf[128], data[128];
	int n;

	tftpctl = open("/net/udp/clone", ORDWR);
	if(tftpctl < 0)
		fatal(1, "open udp clone");

	n = read(tftpctl, buf, sizeof(buf));
	if(n < 0)
		fatal(1, "read clone");
	buf[n] = 0;

	n = write(tftpctl, "announce 69", sizeof("announce 69"));
	if(n < 0)
		fatal(1, "can't announce");

	sprint(data, "/net/udp/%s/data", buf);

	tftpreq = open(data, ORDWR);
	if(tftpreq < 0)
		fatal(1, "open udp/data");

	sprint(data, "/net/udp/%s/remote", buf);
	tftpaddr = open(data, OREAD);
	if(tftpaddr < 0)
		fatal(1, "open udp/remote");

}

void
clrcon(void)
{
	int n;

	n = write(tftpctl, "connect 0.0.0.0!0!r", sizeof("connect 0.0.0.0!0!r"));
	if(n < 0)
		fatal(1, "clear connect");
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

/*
 *  for sun kernel boots, replace the requested file name with
 *  a one from our database.  If the database doesn't specify a file,
 *  don't answer.
 */
char*
sunkernel(char *name)
{
	ulong addr;
	uchar ipaddr[4];
	char buf[32];
	static Ipinfo info;
	static Ndb *db;

	if(strlen(name) != 14 || strncmp(name + 8, ".SUN", 4) != 0)
		return name;

	addr = strtoul(name, 0, 16);
	ipaddr[0] = addr>>24;
	ipaddr[1] = addr>>16;
	ipaddr[2] = addr>>8;
	ipaddr[3] = addr;
	sprint(buf, "%I", ipaddr);
	if(db == 0)
		db = ndbopen(0);
	if(db == 0)
		return 0;
	if(ipinfo(db, 0, buf, 0, &info) < 0)
		return 0;
	if(info.bootf[0])
		return info.bootf;
	return 0;
}
