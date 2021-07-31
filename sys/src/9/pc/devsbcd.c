#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"
#include	"devtab.h"
#include	"io.h"
/*
 * CD-ROM driver for Panasonic and Mitsumi drives on SB16 and SBPRO cards
 */


typedef struct Drive		Drive;

enum
{
	FRAMESIZE	= 2048,		/* max in a transfer */
	Panasonic 	= 1,
	Mitsumi	  	= 2,

	Qdir		= 0,
	Qcd,
	Qcdctl,

	CMDLEN		= 7,

	DTEN		= 0x02,		/* Status register */
	STEN		= 0x04,

	DRIVE0		= 0,

	CMD		= 0,		/* Ports */
	DATA		= 0,
	PHASE		= 1,
	STATUS		= 1,
	RESET 		= 2,
	SELECT		= 3,

	ABORT		= 0x08,
	BLOCKPARAM	= 0x00,
	DOORCLOSE	= 0x07,
	DOOROPEN	= 0x06,
	LOCK		= 0x0c,
	MODESELECT	= 0x09,
	NOP		= 0x05,
	PAUSE		= 0x0d,
	PLAYBLOCKS	= 0x0e,
	PLAYTRKS	= 0x0f,
	READ		= 0x10,
	READDINFO	= 0x8b,
	READERROR	= 0x82,
	READID		= 0x83,
	RESUME		= 0x80,

	ST_DOOROPEN	= 0x80,
	ST_DSKIN	= 0x40,
	ST_SPIN		= 0x20,
	ST_ERROR	= 0x10,
	ST_BUSY		= 0x04,
	ST_AUDIOBSY	= 0x02,

	MEDIA_CHANGED	= 0x11,
	NOT_READY	= 0x03,
	HARD_RESET	= 0x12,
	DISC_OUT	= 0x15,

	MST_CMD_CHECK	= 0x01,		/* command error */
	MST_BUSY	= 0x02,		/* now playing */
	MST_READ_ERR	= 0x04,		/* read error */
	MST_DSK_TYPE	= 0x08,		/* cdda */
	MST_SERVO_CHECK	= 0x10,
	MST_DSK_CHG	= 0x20,		/* disk removed or changed */
	MST_READY	= 0x40,		/* disk in the drive */
	MST_DOOR_OPEN	= 0x80,		/* door is open */

	MFL_DATA	= 0x02,		/* data available */
	MFL_STATUS	= 0x04,		/* status available */

	MCMD_RESET		= 0x00,
	MCMD_GET_DISK_INFO	= 0x10,		/* read info from disk */
	MCMD_GET_Q_CHANNEL	= 0x20,		/* read info from q channel */
	MCMD_GET_STATUS		= 0x40,
	MCMD_SET_MODE		= 0x50,
	MCMD_SOFT_RESET		= 0x60,
	MCMD_STOP		= 0x70,		/* stop play */
	MCMD_CONFIG_DRIVE	= 0x90,
	MCMD_SET_VOLUME		= 0xAE,		/* set audio level */
	MCMD_PLAY_READ		= 0xC0,		/* play or read data */
	MCMD_GET_VERSION  	= 0xDC,
	MCMD_EJECT		= 0xF6,		/* eject (FX drive) */

	MSTAT_DOOR		= 1,
	MSTAT_CHNG,
	MSTAT_CDDA,
	MSTAT_READY,
	MSTAT_ERROR,
};

static char *etable[] =
{
	"no error",
	"soft read error after retry",
	"soft read error after error correction",
	"not ready",
	"unable to read TOC",
	"hard read error of data track",
	"seek failed",
	"tracking servo failure",
	"drive RAM failure",
	"drive self-test failed",
	"focusing servo failure",
	"spindle servo failure",
	"data path failure",
	"illegal logical block address",
	"illegal field in CDB",
	"end of user encountered on this track",
	"illegal data mode for this track",
	"media changed",
	"power-on or drive reset occurred",
	"drive ROM failure",
	"illegal drive command received from host",
	"disc removed during operation",
	"drive Hardware error",
	"illegal request from host"
};

static char *metable[] =
{
	[MSTAT_DOOR]	= "cdrom door is open",
	[MSTAT_CHNG]	= "cdrom media changed",
	[MSTAT_CDDA]	= "cd is not a cdrom",
	[MSTAT_READY]	= "cdrom is not ready",
	[MSTAT_ERROR]	= "cannot get cd status",
};

struct Drive
{
	int		type;
	Ref		opens;
	QLock;
	Rendez		rendez;
	ulong		blocks;			/* blocks on disk */
	int		port;
	uchar		buf[FRAMESIZE];

	void		(*sbcdio)(int, ulong);
	void		(*initdrive)(int);
};

Dirtab
cdtab[] =
{
	"cd",		{Qcd},			0,	0666,
	"cdctl",	{Qcdctl},		0,	0666,
};
#define	Ncdtab		(sizeof cdtab/sizeof(Dirtab))
#define CDFILE		0		/* Array index of Qcd direntry */

static Drive	sbcd;

static void	pansbcdio(int, ulong);
static void	mitsbcdio(int, ulong);
static void	paninitdrive(int);
static void	mitinitdrive(int);

static void
drain(int port)
{
	outb(port+STATUS, 1);
	while((inb(port+STATUS) & (DTEN|STEN)) == STEN)
		inb(port+DATA);
	outb(port+STATUS,0);
}

static int
status(int port)
{
	int sr;

	sr = inb(port+DATA);

	for(;;) {
		if(inb(port+STATUS) == 0xff)
			break;
		if((sr&DTEN|STEN) == STEN)
			drain(port);

		print("devsbcd: busy for sr\n");
		sr = inb(port+DATA);
	}

	return sr;
}

static void
issue(int port, uchar *cmd)
{
	ulong s;
	uchar sr;
	int i, len;

	i = inb(port+STATUS);
	if(i != 0xff) {
		if ((i & DTEN|STEN) == STEN)
			drain(port);
		i = status(port);
		USED(i);
		sr = inb(port+STATUS);
		if(sr != 0xff) {
			print("devsbcd: device wedged, sr=%2.2ux\n", inb(port+STATUS));
			outb(port+RESET, 0);
			tsleep(&sbcd.rendez, return0, 0, 500);
		}
	}

	outb(port+SELECT, DRIVE0);

	len = CMDLEN;
	if(cmd[0] == ABORT)
		len=1;

	s = splhi();
	for (i = 0; i < len; i++)
		outb(port+CMD,*cmd++);
	splx(s);
}

void
sbcdreset(void)
{
}

void
sbcdinit(void)
{
	ISAConf sbconf;

	sbconf.port = 0;
	if(isaconfig("cdrom", 0, &sbconf) == 0)
		return;
	if(strcmp(sbconf.type, "panasonic") == 0 || strcmp(sbconf.type, "matsushita") == 0) {
		sbcd.sbcdio = pansbcdio;
		sbcd.initdrive = paninitdrive;
		if(sbconf.port == 0)
			sbconf.port = 0x230;
	}
	else
	if(strcmp(sbconf.type, "mitsumi") == 0) {
		sbcd.sbcdio = mitsbcdio;
		sbcd.initdrive = mitinitdrive;
		if(sbconf.port == 0)
			sbconf.port = 0x340;
	}
	if(sbcd.sbcdio == 0) {
		print("devsbcd: %s is not a valid cd-rom type\n", sbconf.type);
		return;
	}
	switch(sbconf.port) {
	case 0x230:
	case 0x250:
	case 0x270:
	case 0x290:
	case 0x340:
		break;
	default:
		print("devsbcd: bad port 0x%x\n", sbconf.port);
		return;
	}
	sbcd.port = sbconf.port;	
}

Chan*
sbcdattach(char *param)
{
	if(sbcd.port == 0)
		error("no cd-rom configured");

	return devattach('m', param);
}

Chan*
sbcdclone(Chan *c, Chan *nc)
{
	return devclone(c, nc);
}

int
sbcdwalk(Chan *c, char *name)
{
	return devwalk(c, name, cdtab, Ncdtab, devgen);
}

void
sbcdstat(Chan *c, char *dp)
{
	devstat(c, dp, cdtab, Ncdtab, devgen);
}

static int
poll(int timeo, int state, int port, int delay)
{
	int i, sr;

	sr = 0;
	for(i = 0; i < timeo; i++) {
		sr = inb(port+STATUS) & (STEN|DTEN);
		if(sr != (STEN|DTEN))
			break;
		if(delay != 0)
			tsleep(&sbcd.rendez, return0, 0, TK2MS(1));
	}
	return sr != state;
}

static int
reqsense(int port)
{
	int i;
	uchar cmd[CMDLEN], data[12];

	memset(cmd, 0, sizeof(cmd));
	cmd[0] = READERROR;
	issue(port, cmd);
	i = poll(2000, DTEN, port, 0);
	if(i < 0) {
		print("devsbcd: get error failed\n");
		error(Eio);
	}
	insb(port, data, 8);
	status(port);

	return data[2];
}

static void
getcap(Drive *d)
{
	int port, i, sr, retry;
	uchar cmd[CMDLEN], data[12];

	port = d->port;

	for(retry = 0; retry < 10; retry++) {
		memset(cmd, 0, sizeof(cmd));
		cmd[0] = READDINFO;
		issue(port, cmd);
		i = poll(10, DTEN, port, 1);
		if(i < 0) {
			print("devsbcd: cmd error, sr=%2.2ux\n", status(port));
			error(Eio);
		}
		insb(port, data, 6);
		sr = status(port);

		sbcd.blocks = (data[3]*4500) + (data[4]*75) + data[5];
		sbcd.blocks -= 150;	
		cdtab[CDFILE].length = sbcd.blocks*FRAMESIZE;

		if((sr & ST_ERROR) == 0)	
			break;
	}
	if(retry >= 10)
		error(Eio);
}

Chan*
sbcdopen(Chan *c, int omode)
{
	switch(c->qid.path) {
	case Qcd:
		if(incref(&sbcd.opens) == 1) {
			if(waserror()) {
				decref(&sbcd.opens);
				nexterror();
			}
			sbcd.initdrive(sbcd.port);
			poperror();
		}
		break;
	}
	return devopen(c, omode, cdtab, Ncdtab, devgen);
}

void
sbcdcreate(Chan *c, char *name, int omode, ulong perm)
{
	USED(c, name, omode, perm);
	error(Eperm);
}

void
sbcdclose(Chan *c)
{
	switch(c->qid.path) {
	default:
		break;
	case Qcd:
		if(c->flag & COPEN)
			decref(&sbcd.opens);
		break;
	}
}

void
sbcdremove(Chan *c)
{
	USED(c);
	error(Eperm);
}

void
sbcdwstat(Chan *c, char *dp)
{
	USED(c, dp);
	error(Eperm);
}

long
sbcdread(Chan *c, char *a, long n, ulong offset)
{
	char *t, buf[64];
	long m, o, n0, bn;

	if(c->qid.path & CHDIR)
		return devdirread(c, a, n, cdtab, Ncdtab, devgen);

	n0 = n;
	switch(c->qid.path) {
	case Qcdctl:
		t = "panasonic";
		if(sbcd.sbcdio != pansbcdio)
			t = "mitsumi";
		sprint(buf, "port=0x%ux drive=%s\n", sbcd.port, t);
		return readstr(offset, a, n, buf);
	case Qcd:
		qlock(&sbcd);
		if(waserror()) {
			qunlock(&sbcd);
			nexterror();
		}
		while(n > 0) {
			bn = offset / FRAMESIZE;
			o = offset % FRAMESIZE;
			m = FRAMESIZE - o;
			if(m > n)
				m = n;
			if(bn >= sbcd.blocks)
				break;
			sbcd.sbcdio(sbcd.port, bn);
			memmove(a, sbcd.buf+o, m);
			n -= m;
			offset += m;
			a += m;
		}
		qunlock(&sbcd);
		poperror();
		break;
	}
	return n0-n;
}

long
sbcdwrite(Chan *c, char *a, long n, ulong offset)
{

	USED(c, a, n, offset);
	error(Eperm);
	return 0;
}

static void
bin2bcd(uchar *p)
{
	int u, t;

	u = *p % 10;
	t = *p / 10;
	*p = u | (t << 4);
}

static void
bin2msf(uchar *msf, long addr)
{
	addr += 150;
	msf[0] = addr / 4500;
	addr %= 4500;
	msf[1] = addr / 75;
	msf[2] = addr % 75;
}

static int
bcd2bin(uchar bcd)
{
	return (bcd >> 4) * 10 + (bcd & 0xF);
}

/*
 * panasonic specific
 */
static void
paninitdrive(int port)
{
	int i, sr;
	uchar cmd[CMDLEN];

	qlock(&sbcd);
	if(waserror()) {
		qunlock(&sbcd);
		nexterror();
	}

	memset(cmd, 0, sizeof(cmd));
	cmd[0] = NOP;
	issue(port, cmd);
	i = poll(10, DTEN, port, 1);
	if(i < 0)
		error("cd not responding");

	sr = status(port);
	if((sr&ST_DSKIN) == 0)
		error("no cd in drive");

	if(sr&ST_ERROR) {
		i = reqsense(port);
		switch(i) {
		case NOT_READY:			/* Just media changes */
		case MEDIA_CHANGED:
	    	case HARD_RESET:
	    	case DISC_OUT:
			break;
		default:
			error(etable[i]);	/* Real errors */
		}
	}

	getcap(&sbcd);

	qunlock(&sbcd);
	poperror();
}

static void
pansbcdio(int port, ulong adr)
{
	int sr, i, try, errno;
	uchar *p, msf[3], cmd[CMDLEN];

	bin2msf(msf, adr);

retry:
	memset(cmd, 0, sizeof(cmd));
	cmd[0] = READ;
	cmd[1] = msf[0];
	cmd[2] = msf[1];
	cmd[3] = msf[2];
	cmd[6] = 1;
	issue(port, cmd);

	for(try = 0; try < 100; try++) {
		sr = inb(port+STATUS) & (STEN|DTEN);
		switch(sr) {
		case DTEN|STEN:
			tsleep(&sbcd.rendez, return0, 0, TK2MS(1));
			break;

		case 0:
		case STEN:
			outb(port+STATUS, 1);
			p = sbcd.buf;
			for(i = 0; i < sizeof(sbcd.buf); i++) {
				if(inb(port+STATUS) != 0xfd)
					break;
				*p++ = inb(port+DATA);
			}
			outb(port+STATUS,0);
			while((inb(port+STATUS)&(DTEN|STEN)) != DTEN)
				;
			sr = status(port);
			if(sr & ST_ERROR) {
				errno = reqsense(port);
				error(etable[errno]);
			}
			return;

		case DTEN:
			i = status(port);
			errno = reqsense(port);
			print("devsbcd: %s reading block %d\n", etable[errno], adr);

			switch(errno) {
			case NOT_READY:
			case MEDIA_CHANGED:
	 	  	case HARD_RESET:
	 		case DISC_OUT:
				error(etable[i]);
			}
			goto retry;
		}
	}
}

/*
 * mitsumi specific
 */
static int
statmap(int s)
{
	if(s & MST_DOOR_OPEN)
		return MSTAT_DOOR;	/* door is open */
	if(s & MST_DSK_CHG)
		return MSTAT_CHNG;	/* disk was changed */
	if(s & MST_DSK_TYPE)
		return MSTAT_CDDA;	/* not cdrom type disk */
	if((s & MST_READY) == 0)
		return MSTAT_READY;	/* not ready */
	return 0;
}

static int
mitdata(int port, uchar *d, long n)
{
	int j, f;

	for(j=500; j; j--) {
		f = inb(port+1) & (MFL_DATA|MFL_STATUS);
		if(f == (MFL_DATA|MFL_STATUS)) {
			if(j > 400)
				sched();
			else
				tsleep(&sbcd.rendez, return0, 0, TK2MS(1));
			continue;
		}
		if(f == MFL_STATUS) {
			*d++ = inb(port+0);
			if(n == 1)
				return 0;
			n--;
			j = 500;
			continue;
		}
		break;
	}
	return MSTAT_ERROR;
}

static int
mitstatus(int port, uchar *d, long n)
{
	int j, f;

	for(j=500; j; j--) {
		f = inb(port+1) & (MFL_DATA|MFL_STATUS);
		if(f == (MFL_DATA|MFL_STATUS)) {
			if(j > 400)
				sched();
			else
				tsleep(&sbcd.rendez, return0, 0, TK2MS(1));
			continue;
		}
		if(f == MFL_DATA) {
			*d++ = inb(port+0);
			if(n == 1)
				return 0;
			n--;
			j = 500;
			continue;
		}
		break;
	}
	return MSTAT_ERROR;
}

static void
mitinitdrive(int port)
{
	int i, e;
	uchar dat[10];

	qlock(&sbcd);
	if(waserror()) {
		qunlock(&sbcd);
		nexterror();
	}

	sbcd.blocks = 0;
	for(i=0; i<5; i++) {
		outb(port+0, MCMD_GET_DISK_INFO);
		if(mitstatus(port, dat, 9))
			continue;
		e = statmap(dat[0]);
		if(e)
			continue;
		sbcd.blocks = bcd2bin(dat[3])*4500 +
				bcd2bin(dat[4])*75 +
				bcd2bin(dat[5]) -
				150;
		cdtab[CDFILE].length = sbcd.blocks*FRAMESIZE;

		break;
	}
	if(sbcd.blocks == 0)
		error("cant issue capacity");

	qunlock(&sbcd);
	poperror();
}

static void
del(void)
{
	int i;

	for(i=0; i<10; i++)
		;
}

static void
mitsbcdio(int port, ulong adr)
{
	int i, e;
	ulong s;
	uchar status, msf[3];

	bin2msf(msf, adr);
	bin2bcd(msf+0);		/* convert to BCD */
	bin2bcd(msf+1);
	bin2bcd(msf+2);

	e = 0;
	for(i=0; i<5; i++) {

		s = splhi();
		outb(port+0, MCMD_SET_MODE);
		outb(port+0, 1);	/* cooked data */
		splx(s);

		if(mitstatus(port, &status, 1))
			continue;
		e = statmap(status);
		if(e)
			continue;

		s = splhi();
		outb(port+0, MCMD_PLAY_READ);
		outb(port+0, msf[0]);
		outb(port+0, msf[1]);
		outb(port+0, msf[2]);
		outb(port+0, 0);
		outb(port+0, 0);
		outb(port+0, 1);
		splx(s);

		if(mitdata(port, sbcd.buf, FRAMESIZE) == 0) {
			if(mitstatus(port, &status, 1))
				continue;
			e = statmap(status);
			if(e)
				continue;
			return;
		}
	}
	if(e == 0)
		e = MSTAT_ERROR;
	error(metable[e]);
}
