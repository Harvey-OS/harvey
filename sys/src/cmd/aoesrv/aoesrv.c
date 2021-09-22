/*
 * aoesrv -- serve data via ATA-over-Ethernet
 * original copyright © 2007 erik quanstrom
 */

#include <u.h>
#include <libc.h>
#include <thread.h>
#include <ip.h>			/* irony */

enum {
	Eaddrlen	= 6,	/* only defined in kernel. */
};
#include "aoe.h"

/* ata errors (stolen from ahci.h) */

enum {
	Emed	= 1<<0,		/* media error */
	Enm	= 1<<1,		/* no media */
	Eabrt	= 1<<2,		/* abort */
	Emcr	= 1<<3,		/* media change request */
	Eidnf	= 1<<4,		/* no user-accessible address */
	Emc	= 1<<5,		/* media change */
	Eunc	= 1<<6,		/* data error */
	Ewp	= 1<<6,		/* write protect */
	Eicrc	= 1<<7,		/* interface crc error */

	Efatal	= Eidnf|Eicrc,	/* must sw reset. */
};

/* ata status */
enum {
	ASerr	= 1<<0,		/* error */
	ASdrq	= 1<<3,		/* request */
	ASdf	= 1<<5,		/* fault */
	ASdrdy	= 1<<6,		/* ready */
	ASbsy	= 1<<7,		/* busy */

	ASobs	= 1<<1|1<<2|1<<4,
};

enum {
	Fclone,
	Fdata,
	Flast,
};

enum {
	Nether	= 5,
	Maxpkt	= 10000,
	Hdrlba	= 128,
	Conflen	= 1024,
};

#define MAGIC	"aoe vblade\n"

typedef struct {
	char	magic[32];
	char	size[32];
	char	address[16];
	char	configlen[6];
	char	pad[512-32-32-16-6];
	char	config[Conflen];
} Vbhdr;

typedef struct {
	Vbhdr	hdr;
	vlong	maxlba;
	int	shelf, slot;
	int	clen;
} Vblade;

static	Vblade	vblade;

static	int	fd;
static	char	*ethertab[Nether] = { "/net/ether0", };
static	int	etheridx = 1;
static	int	efdtab[Nether*Flast];
static	char	pkttab[Nether][Maxpkt];
static	int	mtutab[Nether];

static int
getmtu(char *p)
{
	int fd, mtu;
	char buf[50];

	snprint(buf, sizeof buf, "%s/mtu", p);
	if((fd = open(buf, OREAD)) == -1 || read(fd, buf, 36) < 0) {
		close(fd);
		return 2;
	}
	close(fd);
	buf[36] = 0;
	mtu = strtoul(buf+12, 0, 0) - sizeof(Aoehdr);
	return mtu >> 9;
}

int
parseshelf(char *s, int *shelf, int *slot)
{
	int a, b;

	a = strtoul(s, &s, 0);
	if(*s++ != '.')
		return -1;
	b = strtoul(s, &s, 0);
	if(*s != 0)
		return -1;
	*shelf = a;
	*slot = b;
	return 0;
}

static vlong
getsize(char *s)
{
	vlong v;
	char *p;
	static char tab[] = "ptgmk";

	v = strtoull(s, &s, 0);
	while((p = strchr(tab, *s++)) != nil && *p != '\0')
		while(*p++)
			v *= 1024;
	if(s[-1] != '\0')
		return -1;
	return v;
}

vlong
sizetolba(vlong size)
{
	if(size < 512 || size & 0x1ff){
		fprint(2, "%s: bad size %lld\n", argv0, size);
		exits("size");
	}
	return size >> 9;
}

static int
savevblade(int fd, Vblade *vb)
{
	int n, r;
	char *p;

	seek(fd, 0, 0);
	p = (char*)vb;
	for(n = 0; n < sizeof *vb; n += r)
		if((r = write(fd, p + n, sizeof *vb - n)) <= 0)
			break;
	if(n != sizeof *vb)
		return -1;
	return 0;
}

static char*
chkvblade(int fd, Vblade *vb)
{
	Vbhdr *h;

	h = &vb->hdr;
	if(readn(fd, (char*)h, sizeof *h) != sizeof *h)
		return "bad read";
	if(memcmp(h->magic, MAGIC, sizeof MAGIC) != 0)
		return "bad magic";
	h->size[sizeof h->size - 1] = 0;
	vb->maxlba = sizetolba(strtoull(h->size, 0, 0));
	if(parseshelf(h->address, &vb->shelf, &vb->slot) == -1)
		return "bad shelf";
	h->configlen[sizeof h->configlen - 1] = 0;
	vb->clen = strtoul(h->configlen, 0, 0);
	return 0;
}

void
checkfile(char *s, Vblade *vb, int iflag)
{
	char *e;

	fd = open(s, ORDWR);
	if(fd == -1)
		sysfatal("can't open backing store %s: %r", s);
	if(iflag && (e = chkvblade(fd, vb)) != nil)
		sysfatal("invalid vblade %s", e);
}

void
recheck(int fd, Vblade *vb)
{
	vlong v;
	Dir *d;

	d = dirfstat(fd);
	if(d == nil)
		sysfatal("can't stat: %r");
	v = sizetolba(d->length & ~0x1ff) - Hdrlba;
	free(d);
	if(vb->maxlba >= v)
		sysfatal("cmdline size too large (%d sector overhead)", Hdrlba);
	if(vb->maxlba == 0)
		vb->maxlba = v;

	sprint(vb->hdr.size, "%lld", vb->maxlba << 9);
	sprint(vb->hdr.address, "%d.%d", vb->shelf, vb->slot);
	sprint(vb->hdr.configlen, "%d", vb->clen);

	savevblade(fd, vb);
}

int
aoeopen(char *e, int fds[])
{
	int n;
	char buf[128], ctl[13];

	snprint(buf, sizeof buf, "%s/clone", e);
	if((fds[Fclone] = open(buf, ORDWR)) == -1) {
		werrstr("can't open %s: %r", buf);
		return -1;
	}
	memset(ctl, 0, sizeof ctl);
	if(read(fds[Fclone], ctl, sizeof ctl) < 0) {
		werrstr("read error on %s: %r", buf);
		return -1;
	}
	n = atoi(ctl);
	snprint(buf, sizeof buf, "connect %d", Aoetype);
	if(write(fds[Fclone], buf, strlen(buf)) != strlen(buf)) {
		werrstr("write error: %r");
		return -1;
	}
	snprint(buf, sizeof buf, "%s/%d/data", e, n);
	fds[Fdata] = open(buf, ORDWR);
	if (fds[Fdata] < 0)
		werrstr("can't open %s: %r", buf);
	return fds[Fdata];
}

void
replyhdr(Aoehdr *h)
{
	uchar ea[Eaddrlen];

	memmove(ea, h->dst, Eaddrlen);
	memmove(h->dst, h->src, Eaddrlen);
	memmove(h->src, ea, Eaddrlen);

	hnputs(h->major, vblade.shelf);
	h->minor = vblade.slot;
	h->verflag |= AFrsp;
}

static int
serveconfig(Aoeqc *q, Vblade *vb, int mtu)
{
	int cmd, reqlen, len;
	char *cfg;

	if(memcmp(q->src, q->dst, Eaddrlen) == 0)
		return -1;

	reqlen = nhgets(q->cslen);
	len = vb->clen;
	cmd = q->verccmd&0xf;
	cfg = (char*)(q+1);

	switch(cmd){
	case AQCtest:
		if(reqlen != len)
			return -1;
		/* fall through */
	case AQCprefix:
		if(reqlen > len || memcmp(vb->hdr.config, cfg, reqlen) != 0)
			return -1;
		break;
	case AQCread:
		break;
	case AQCset:
		if(len && len != reqlen ||
		    memcmp(vb->hdr.config, cfg, reqlen) != 0){
			q->verflag |= AFerr;
			q->error = AEcfg;
			break;
		}
		/* fall through */
	case AQCfset:
		if(reqlen > Conflen){
			q->verflag |= AFerr;
			q->error = AEarg;
			break;
		}
		memmove(vb->hdr.config, cfg, reqlen);
		vb->clen = len = reqlen;
		break;
	default:
		q->verflag |= AFerr;
		q->error = AEarg;
		break;
	}

	memmove(cfg, vb->hdr.config, len);
	hnputs(q->cslen, len);
	hnputs(q->bufcnt, 24);
	q->scnt = mtu;
	hnputs(q->fwver, 2323);
	q->verccmd = Aoever<<4 | cmd;

	return len + sizeof *q;
}

static ushort ident[256] = {
	[47] 0x8000,
	[49] 0x0200,
	[50] 0x4000,
	[83] 0x5400,
	[84] 0x4000,
	[86] 0x1400,
	[87] 0x4000,
	[93] 0x400b,
};

static void
idmove(char *a, int idx, int len, char *s)
{
	char *p;

	p = a + idx*2;
	for(; len > 0; len -= 2) {
		if(*s == 0)
			p[1] = ' ';
		else
			p[1] = *s++;
		if (*s == 0)
			p[0] = ' ';
		else
			p[0] = *s++;
		p += 2;
	}
}

static void
lbamove(char *p, int idx, int n, vlong lba)
{
	int i;

	p += idx*2;
	for(i = 0; i < n; i++)
		*p++ = lba >> (i*8);
}

enum {
	Crd		= 0x20,
	Crdext		= 0x24,
	Cwr		= 0x30,
	Cwrext		= 0x34,
	Cid		= 0xec,
};

static vlong
getlba(uchar *p)
{
	vlong v;

	v  = p[0];
	v |= p[1] << 8;
	v |= p[2] << 16;
	v |= p[3] << 24;
	v |= (vlong)p[4] << 32;
	v |= (vlong)p[5] << 40;
	return v;
}

static void
putlba(uchar *p, vlong lba)
{
	p[0] = lba;
	p[1] = lba >> 8;
	p[2] = lba >> 16;
	p[3] = lba >> 24;
	p[5] = lba >> 32;
	p[6] = lba >> 40;
}

static int
serveata(Aoeata *a, Vblade *vb, int mtu)
{
	int rbytes, bytes, len;
	vlong lba, off;
	char *buf;

	buf = (char*)(a + 1);
	lba = getlba(a->lba);
	len = a->scnt << 9;
	off = (lba + Hdrlba) << 9;

	if(a->scnt > mtu || a->scnt == 0){
		a->verflag |= AFerr;
		a->error = AEarg;
		return 0;
	}

	if(a->cmdstat != Cid && lba + a->scnt > vb->maxlba){
		a->errfeat = Eidnf;
		a->cmdstat = ASdrdy|ASerr;
		return 0;
	}

	if((a->cmdstat & 0xf0) == 0x20)
		lba &= 0xfffffff;
	switch(a->cmdstat){
	default:
		a->errfeat = Eabrt;
		a->cmdstat = ASdrdy | ASerr;
		return 0;
	case Cid:
		memmove(buf, ident, sizeof ident);
		idmove(buf, 27, 40, "Plan 9 aoesrv");
		idmove(buf, 23, 8, "fwver");
		idmove(buf, 23, 8, "2");
		lbamove(buf, 60, 4, vb->maxlba);
		lbamove(buf, 100, 8, vb->maxlba);
		a->cmdstat = ASdrdy;
		return 512;
		break;
	case Crd:
	case Crdext:
		bytes = pread(fd, buf, len, off);
		rbytes = bytes;
		break;
	case Cwr:
	case Cwrext:
		bytes = pwrite(fd, buf, len, off);
		rbytes = 0;
		break;
	}
	if(bytes != len){
		a->errfeat = Eabrt;
		a->cmdstat = ASdf | ASerr;
		putlba(a->lba, (lba + (len - bytes)) >> 9);
		return 0;
	}

	putlba(a->lba, lba + a->scnt);
	a->scnt = 0;
	a->errfeat = 0;
	a->cmdstat = ASdrdy;

	return rbytes;
}

static int
myea(uchar ea[6], char *p)
{
	int fd;
	char buf[50];

	snprint(buf, sizeof buf, "%s/addr", p);
	if((fd = open(buf, OREAD)) == -1 || read(fd, buf, 12) < 12) {
		close(fd);
		return -1;
	}
	close(fd);
	return parseether(ea, buf);
}

static void
bcastpkt(Aoeqc *h, Vblade *vb, int i)
{
	myea(h->dst, ethertab[i]);
	memset(h->src, 0xff, Eaddrlen);
	hnputs(h->type, Aoetype);
	hnputs(h->major, vb->shelf);
	h->minor = vb->slot;
	h->cmd = ACconfig;
	*(u32int*)h->tag = 0;
}

void
serve(void *v)
{
	int i, n, efd, mtu;
	char *pkt;
	Aoehdr *h;

	i = (int)(uintptr)v;

	pkt = pkttab[i];
	efd = efdtab[i*Flast + Fdata];
	mtu = mtutab[i];

	bcastpkt((Aoeqc*)pkt, &vblade, i);
	for(;;){
		h = (Aoehdr*)pkt;
		switch(h->cmd){
		case ACata:
			n = serveata((Aoeata*)pkt, &vblade, mtu);
			n += sizeof(Aoeata);
			break;
		case ACconfig:
			n = serveconfig((Aoeqc*)pkt, &vblade, mtu);
			break;
		default:
			n = -1;		/* this cmd is rubbish, don't reply */
			break;
		}
		if(n != -1) {		/* any response worth sending? */
			replyhdr(h);
			if(n < 60)
				n = 60;
			if(write(efd, pkt, n) != n){
				fprint(2, "%s: write to %s failed: %r\n", argv0,
					ethertab[i]);
				break;
			}
		}

		/* read commands, ignore runts */
		while (read(efd, pkt, Maxpkt) < 60)
			continue;
	}
}

void
launch(char *tab[], int fdtab[])
{
	int i;

	for(i = 0; tab[i]; i++){
		if(aoeopen(tab[i], fdtab + Flast*i) < 0)
			sysfatal("%s: network open: %r", tab[i]);
		/*
		 * use proc not threads.  otherwise we will block on read/write.
		 */
		proccreate(serve, (void*)i, 32*1024);
	}
}

void
usage(void)
{
	fprint(2, "%s [-i] [-s size] [-a shelf.slot] [-c config] "
		"[-e ether] file\n", argv0);
	exits("usage");
}

void
threadmain(int argc, char **argv)
{
	int flagi, shelf, slot, i, anye;
	vlong maxlba;
	char *config, *anal;

	shelf = -1;
	config = 0;
	maxlba = 0;
	flagi = 0;
	anye = 0;

	ARGBEGIN{
	case 'a':
		if(parseshelf(EARGF(usage()), &shelf, &slot) == -1)
			sysfatal("bad vblade address");
		break;
	case 'e':
		if(anye++ == 0)
			etheridx = 0;
		if(etheridx == nelem(ethertab))
			sysfatal("too many interfaces");
		ethertab[etheridx++] = EARGF(usage());
		break;
	case 's':
		maxlba = sizetolba(getsize(EARGF(usage())));
		break;
	case 'c':
		config = EARGF(usage());
		break;
	case 'i':
		flagi++;
		break;
	default:
		usage();
	}ARGEND;

	if(argc != 1)
		usage();
	if(flagi == 0)
		checkfile(*argv, &vblade, 1);
	else{
		memcpy(vblade.hdr.magic, MAGIC, sizeof MAGIC);
		checkfile(*argv, &vblade, 0);
	}

	if(shelf != -1){
		vblade.shelf = shelf;
		vblade.slot = slot;
	}
	if(maxlba > 0)
		vblade.maxlba = maxlba;
	if(config)
		memmove(vblade.hdr.config, config, vblade.clen = strlen(config));

	recheck(fd, &vblade);

	for(i = 0; i < etheridx; i++)
		mtutab[i] = getmtu(ethertab[i]);

	anal = "";
	if(vblade.maxlba > 1)
		anal = "s";
	fprint(2, "lblade %d.%d %lld sector%s\n", vblade.shelf, vblade.slot,
		vblade.maxlba, anal);

	launch(ethertab, efdtab);

	while(sleep(1000) != -1)
		;
	threadexitsall("interrupted");
}
