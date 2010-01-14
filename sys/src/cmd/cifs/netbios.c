/*
 * netbios dial, read, write
 */

#include <u.h>
#include <libc.h>
#include <ctype.h>
#include <fcall.h>
#include <thread.h>
#include <9p.h>
#include "cifs.h"

enum {
	MAXNBPKT		= 8096,		/* max netbios packet size */
	NBquery			= 0,		/* packet type - query */

	NBAdapterStatus		= 0x21,		/* get host interface info */
	NBInternet		= 1,		/* scope for info */

	NBmessage 		= 0x00,		/* Netbios packet types */
	NBrequest 		= 0x81,
	NBpositive,
	NBnegative,
	NBretarget,
	NBkeepalive,

	ISgroup			= 0x8000,
};


static char *NBerr[] = {
	[0]	"not listening on called name",
	[1]	"not listening for calling name",
	[2]	"called name not present",
	[3]	"insufficient resources",
	[15]	"unspecified error"
};


static ulong
GL32(uchar **p)
{
	ulong n;

	n  = *(*p)++;
	n |= *(*p)++ << 8;
	n |= *(*p)++ << 16;
	n |= *(*p)++ << 24;
	return n;
}

static ushort
GL16(uchar **p)
{
	ushort n;

	n  = *(*p)++;
	n |= *(*p)++ << 8;
	return n;
}

void
Gmem(uchar **p, void *v, int n)
{
	uchar *str = v;

	while(n--)
		*str++ = *(*p)++;
}


static ulong
GB32(uchar **p)
{
	ulong n;

	n  = *(*p)++ << 24;
	n |= *(*p)++ << 16;
	n |= *(*p)++ << 8;
	n |= *(*p)++;
	return n;
}

static ushort
GB16(uchar **p)
{
	ushort n;

	n  = *(*p)++ << 8;
	n |= *(*p)++;
	return n;
}

static uchar
G8(uchar **p)
{
	return *(*p)++;
}

static void
PB16(uchar **p, uint n)
{
	*(*p)++ = n >> 8;
	*(*p)++ = n;
}

static void
P8(uchar **p, uint n)
{
	*(*p)++ = n;
}


static void
nbname(uchar **p, char *name, char pad)
{
	char c;
	int i;
	int done = 0;

	*(*p)++ = 0x20;
	for(i = 0; i < 16; i++) {
		c = pad;
		if(!done && name[i] == '\0')
			done = 1;
		if(!done)
			c = toupper(name[i]);
		*(*p)++ = ((uchar)c >> 4) + 'A';
		*(*p)++ = (c & 0xf) + 'A';
	}
	*(*p)++ = 0;
}

int
calledname(char *host, char *name)
{
	char *addr;
	uchar buf[1024], *p;
	static char tmp[20];
	int num, flg, svs, j, i, fd, trn;

	trn = (getpid() ^ time(0)) & 0xffff;
	if((addr = netmkaddr(host, "udp", "137")) == nil)
		return -1;

	if((fd = dial(addr, "137", 0, 0)) < 0)
		return -1;
	p = buf;

	PB16(&p, trn);			/* TRNid */
	P8(&p, 0);			/* flags */
	P8(&p, 0x10);			/* type */
	PB16(&p, 1);			/* # questions */
	PB16(&p, 0);			/* # answers */
	PB16(&p, 0);			/* # authority RRs */
	PB16(&p, 0);			/* # Aditional RRs */
	nbname(&p, "*", 0);
	PB16(&p, NBAdapterStatus);
	PB16(&p, NBInternet);

	if(Debug && strstr(Debug, "dump"))
		xd(nil, buf, p-buf);

	if(write(fd, buf, p-buf) != p-buf)
		return -1;

	p = buf;
	for(i = 0; i < 3; i++){
		memset(buf, 0, sizeof(buf));
		alarm(NBNSTOUT);
		read(fd, buf, sizeof(buf));
		alarm(0);
		if(GB16(&p) == trn)
			break;
	}
	close(fd);
	if(i >= 3)
		return -1;

	p = buf +56;
	num = G8(&p);			/* number of names */

	for(i = 0; i < num; i++){
		memset(tmp, 0, sizeof(tmp));
		Gmem(&p, tmp, 15);
		svs = G8(&p);
		flg = GB16(&p);
		for(j = 14; j >= 0 && tmp[j] == ' '; j--)
			tmp[j] = 0;
		if(svs == 0 && !(flg & ISgroup))
			strcpy(name, tmp);
	}
	return 0;
}


int
nbtdial(char *addr, char *called, char *sysname)
{
	char redir[20];
	uchar *p, *lenp, buf[1024];
	int type, len, err, fd, nkeepalive, nretarg;

	nretarg = 0;
	nkeepalive = 0;
Redial:
	if((addr = netmkaddr(addr, "tcp", "139")) == nil ||
	    (fd = dial(addr, 0, 0, 0)) < 0)
		return -1;

	memset(buf, 0, sizeof(buf));

	p = buf;
	P8(&p, NBrequest);		/* type */
	P8(&p, 0);			/* flags */
	lenp = p; PB16(&p, 0);		/* length placeholder */
	nbname(&p, called, ' ');	/* remote NetBios name */
	nbname(&p, sysname, ' ');	/* our machine name */
	PB16(&lenp, p-lenp -2);		/* length re-write */

	if(Debug && strstr(Debug, "dump"))
		xd(nil, buf, p-buf);
	if(write(fd, buf, p-buf) != p-buf)
		goto Error;
Reread:
	p = buf;
	memset(buf, 0, sizeof(buf));
	if(readn(fd, buf, 4) < 4)
		goto Error;

	type = G8(&p);
	G8(&p);				/* flags */
	len = GB16(&p);

	if(readn(fd, buf +4, len -4) < len -4)
		goto Error;

	if(Debug && strstr(Debug, "dump"))
		xd(nil, buf, len+4);

	switch(type) {
	case NBpositive:
		return fd;
	case NBnegative:
		if(len < 1) {
			werrstr("nbdial: bad error pkt");
			goto Error;
		}
		err = G8(&p);
		if(err < 0 || err > nelem(NBerr) || NBerr[err] == nil)
			werrstr("NBT: %d - unknown error", err);
		else
			werrstr("NBT: %s", NBerr[err]);

		goto Error;
	case NBkeepalive:
		if(++nkeepalive >= 16){
			werrstr("nbdial: too many keepalives");
			goto Error;
		}
		goto Reread;

	case NBretarget:
		if(++nretarg >= 16) {
			werrstr("nbdial: too many redirects");
			goto Error;
		}
		if(len < 4) {
			werrstr("nbdial: bad redirect pkt");
			goto Error;
		}
		sprint(redir, "%d.%d.%d.%d", p[0], p[1], p[2], p[3]);
		addr = redir;
		goto Redial;

	default:
		werrstr("nbdial: 0x%x - unknown packet in netbios handshake", type);
		goto Error;
	}
Error:
	close(fd);
	return -1;
}

void
nbthdr(Pkt *p)
{
	p->pos = p->buf;
	memset(p->buf, 0xa5, MTU);

	p8(p, NBmessage);		/* type */
	p8(p, 0);			/* flags */
	pb16(p, 0);			/* length (filled in later) */
}

int
nbtrpc(Pkt *p)
{
	int len, got, type, nkeep;

	len = p->pos - p->buf;

	p->pos = p->buf +2;
	pb16(p, len - NBHDRLEN);	/* length */

	if(Debug && strstr(Debug, "dump"))
		xd("tx", p->buf, len);

	alarm(NBRPCTOUT);
	if(write(p->s->fd, p->buf, len) != len){
		werrstr("nbtrpc: write failed - %r");
		alarm(0);
		return -1;
	}

	nkeep = 0;
retry:
	p->pos = p->buf;
	memset(p->buf, 0xa5, MTU);

	got = readn(p->s->fd, p->buf, NBHDRLEN);

	if(got < NBHDRLEN){
		werrstr("nbtrpc: short read - %r");
		alarm(0);
		return -1;
	}
	p->eop = p->buf + got;

	type = g8(p);			/* NBT type (session) */
	if(type == NBkeepalive){
		if(++nkeep > 16) {
			werrstr("nbtrpc: too many keepalives (%d attempts)", nkeep);
			alarm(0);
			return -1;
		}
		goto retry;
	}

	g8(p);				/* NBT flags (none) */

	len = gb16(p);			/* NBT payload length */
	if((len +NBHDRLEN) > MTU){
		werrstr("nbtrpc: packet bigger than MTU, (%d > %d)", len, MTU);
		alarm(0);
		return -1;
	}

	got = readn(p->s->fd, p->buf +NBHDRLEN, len);
	alarm(0);

	if(Debug && strstr(Debug, "dump"))
		xd("rx", p->buf, got +NBHDRLEN);

	if(got < 0)
		return -1;
	p->eop = p->buf + got +NBHDRLEN;
	return got+NBHDRLEN;
}


void
xd(char *str, void *buf, int n)
{
	int fd, flg, flags2, cmd;
	uint sum;
	long err;
	uchar *p, *end;

	if(n == 0)
		return;

	p = buf;
	end = (uchar *)buf +n;

	if(Debug && strstr(Debug, "log") != nil){
		if((fd = open("pkt.log", ORDWR)) == -1)
			return;
		seek(fd, 0, 2);
		fprint(fd, "%d	", 0);
		while(p < end)
			fprint(fd, "%02x ", *p++);
		fprint(fd, "\n");
		close(fd);
		return;
	}

	if(!str)
		goto Raw;

	p = (uchar *)buf + 4;
	if(GL32(&p) == 0x424d53ff){
		buf = (uchar *)buf + 4;
		n -= 4;
	}
	end = (uchar *)buf + n;

	sum = 0;
	p = buf;
	while(p < end)
		sum += *p++;
	p = buf;

	fprint(2, "%s : len=%ud sum=%d\n", str, n, sum);

	fprint(2, "mag=0x%ulx ", GL32(&p));
	fprint(2, "cmd=0x%ux ", cmd = G8(&p));
	fprint(2, "err=0x%ulx ", err=GL32(&p));
	fprint(2, "flg=0x%02ux ", flg = G8(&p));
	fprint(2, "flg2=0x%04ux\n", flags2= GL16(&p));
	fprint(2, "dfs=%s\n", (flags2 & FL2_DFS)? "y": "n");

	fprint(2, "pidl=%ud ", GL16(&p));
	fprint(2, "res=%uld ", GL32(&p));
	fprint(2, "sid=%ud ", GL16(&p));
	fprint(2, "seq=0x%ux ", GL16(&p));
	fprint(2, "pad=%ud ", GL16(&p));

	fprint(2, "tid=%ud ", GL16(&p));
	fprint(2, "pid=%ud ", GL16(&p));
	fprint(2, "uid=%ud ", GL16(&p));
	fprint(2, "mid=%ud\n", GL16(&p));

	if(cmd == 0x32 && (flg & 0x80) == 0){		/* TRANS 2, TX */
		fprint(2, "words=%ud ", G8(&p));
		fprint(2, "totparams=%ud ", GL16(&p));
		fprint(2, "totdata=%ud ", GL16(&p));
		fprint(2, "maxparam=%ud ", GL16(&p));
		fprint(2, "maxdata=%ud\n", GL16(&p));
		fprint(2, "maxsetup=%ud ", G8(&p));
		fprint(2, "reserved=%ud ", G8(&p));
		fprint(2, "flags=%ud ", GL16(&p));
		fprint(2, "timeout=%uld\n", GL32(&p));
		fprint(2, "reserved=%ud ", GL16(&p));
		fprint(2, "paramcnt=%ud ", GL16(&p));
		fprint(2, "paramoff=%ud ", GL16(&p));
		fprint(2, "datacnt=%ud ", GL16(&p));
		fprint(2, "dataoff=%ud ", GL16(&p));
		fprint(2, "setupcnt=%ud ", G8(&p));
		fprint(2, "reserved=%ud\n", G8(&p));
		fprint(2, "trans2=0x%02x ", GL16(&p));
		fprint(2, "data-words=%d ", G8(&p));
		fprint(2, "padding=%d\n", G8(&p));
	}
	if(cmd == 0x32 && (flg & 0x80) == 0x80){	/* TRANS 2, RX */
		fprint(2, "words=%ud ", G8(&p));
		fprint(2, "totparams=%ud ", GL16(&p));
		fprint(2, "totdata=%ud ", GL16(&p));
		fprint(2, "reserved=%ud ", GL16(&p));
		fprint(2, "paramcnt=%ud\n", GL16(&p));
		fprint(2, "paramoff=%ud ", GL16(&p));
		fprint(2, "paramdisp=%ud ", GL16(&p));
		fprint(2, "datacnt=%ud\n", GL16(&p));
		fprint(2, "dataoff=%ud ", GL16(&p));
		fprint(2, "datadisp=%ud ", GL16(&p));
		fprint(2, "setupcnt=%ud ", G8(&p));
		fprint(2, "reserved=%ud\n", G8(&p));
	}
	if(err)
		if(flags2 & FL2_NT_ERRCODES)
			fprint(2, "err=%s\n", nterrstr(err));
		else
			fprint(2, "err=%s\n", doserrstr(err));
Raw:
	fprint(2, "\n");
	for(; p < end; p++){
		if((p - (uchar *)buf) % 16 == 0)
			fprint(2, "\n%06lx\t", p - (uchar *)buf);
		if(isprint((char)*p))
			fprint(2, "%c  ", (char )*p);
		else
			fprint(2, "%02ux ", *p);
	}
	fprint(2, "\n");
}
