#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"../port/error.h"

enum{
	Qdir,
	Qbacklight,
	Qbattery,
	Qbuttons,
	Qcruft,
	Qkbdin,
	Qled,
	Qversion,
	Qpower,

	/* command types */
	BLversion=	0,
	BLbuttons=	2,	/* button events */
	BLtouch=		3,	/* read touch screen events */
	BLled=		8,	/* turn LED on/off */
	BLbattery=	9,	/* read battery status */
	BLbacklight=	0xd,	/* backlight control */

	SOF=	0x2,		/* start of frame */
};

/* from /sys/include/keyboard.h */
enum {
	KF=	0xF000,	/* Rune: beginning of private Unicode space */
	/* KF|1, KF|2, ..., KF|0xC is F1, F2, ..., F12 */
	Khome=	KF|0x0D,
	Kup=	KF|0x0E,
	Kpgup=	KF|0x0F,
	Kprint=	KF|0x10,
	Kleft=	KF|0x11,
	Kright=	KF|0x12,
	Kdown=	0x80,
	Kview=	0x80,
	Kpgdown=	KF|0x13,
	Kins=	KF|0x14,
	Kend=	'\r',	/* [sic] */

	Kalt=		KF|0x15,
	Kshift=	KF|0x16,
	Kctl=		KF|0x17,
};

Dirtab µcdir[]={
	".",			{ Qdir, 0, QTDIR },	0,	DMDIR|0755,
	"backlight",		{ Qbacklight, 0 },	0,	0664,
	"battery",		{ Qbattery, 0 },		0,	0664,
	"buttons",		{ Qbuttons, 0 },		0,	0664,
	"cruft",		{ Qcruft, 0 },		0,	0664,
	"kbdin",		{ Qkbdin, 0 },		0,	0664,
	"led",			{ Qled, 0 },			0,	0664,
	"version",		{ Qversion, 0 },		0,	0664,
	"power",		{ Qpower, 0 },		0,	0600,
};

static struct µcontroller
{
	/* message being rcvd */
	int		state;
	uchar	buf[16+4];
	uchar	n;

	/* for messages that require acks */
	QLock;
	Rendez	r;

	/* battery */
	uchar	acstatus;
	uchar	voltage;
	ushort	batstatus;
	uchar	batchem;

	/* version string */
	char	version[16+2];
} ctlr;

/* button map */
Rune bmap[2][4] = 
{
	{Kup, Kright, Kleft, Kdown},	/* portrait mode */
	{Kright, Kdown, Kup, Kleft},	/* landscape mode */
};

extern int landscape;

int
µcputc(Queue*, int ch)
{
	int i, len, b, up;
	uchar cksum;
	uchar *p;
	static int samseq;
	static int touching;	/* guard against something we call going spllo() */
	static int buttoning;	/* guard against something we call going spllo() */

	if(ctlr.n > sizeof(ctlr.buf))
		panic("µcputc");

	ctlr.buf[ctlr.n++] = (uchar)ch;

	for(;;){
		/* message hasn't started yet? */
		if(ctlr.buf[0] != SOF){
			p = memchr(ctlr.buf, SOF, ctlr.n);
			if(p == nil){
				ctlr.n = 0;
				break;
			} else {
				ctlr.n -= p-ctlr.buf;
				memmove(ctlr.buf, p, ctlr.n);
			}
		}
	
		/* whole msg? */
		len = ctlr.buf[1] & 0xf;
		if(ctlr.n < 3 || ctlr.n < len+3)
			break;
	
		/* check the sum */
		ctlr.buf[0] = ~SOF;	/* make sure we process this msg exactly once */
		cksum = 0;
		for(i = 1; i < len+2; i++)
			cksum += ctlr.buf[i];
		if(ctlr.buf[len+2] != cksum)
			continue;
	
		/* parse resulting message */
		p = ctlr.buf+2;
		switch(ctlr.buf[1] >> 4){
		case BLversion:
			strncpy(ctlr.version, (char*)p, len);
			ctlr.version[len] = '0';
			strcat(ctlr.version, "\n");
			wakeup(&ctlr.r);
			break;
		case BLbuttons:
			if(len < 1 || buttoning)
				break;
			buttoning = 1;
			b = p[0] & 0x7f;
			up = p[0] & 0x80;

			if(b > 5) {
				/* rocker panel acts like arrow keys */
				if(b < 10 && !up)
					kbdputc(kbdq, bmap[landscape][b-6]);
			} else {
				/* the rest like mouse buttons */
				if(--b == 0)
					b = 5;
				penbutton(up, 1<<b);
			}
			buttoning = 0;
			break;
		case BLtouch:
			if(touching)
				break;
			touching = 1;
			if(len == 4) {
				if (samseq++ > 10){
					if (landscape)
						pentrackxy((p[0]<<8)|p[1], (p[2]<<8)|p[3]);
					else
						pentrackxy((p[2]<<8)|p[3], (p[0]<<8)|p[1]);
				}
			} else {
				samseq = 0;
				pentrackxy(-1, -1);
			}
			touching = 0;
			break;
		case BLled:
			wakeup(&ctlr.r);
			break;
		case BLbattery:
			if(len >= 5){
				ctlr.acstatus = p[0];
				ctlr.voltage = (p[3]<<8)|p[2];
				ctlr.batstatus = p[4];
				ctlr.batchem = p[1];
			}
			wakeup(&ctlr.r);
			break;
		case BLbacklight:
			wakeup(&ctlr.r);
			break;
		default:
			print("unknown µc message: %ux", ctlr.buf[1] >> 4);
			for(i = 0; i < len; i++)
				print(" %ux", p[i]);
			print("\n");
			break;
		}
	
		/* remove the message */
		ctlr.n -= len+3;
		memmove(ctlr.buf, &ctlr.buf[len+3], ctlr.n);
	}
	return 0;
}

static void
_sendmsg(uchar id, uchar *data, int len)
{
	uchar buf[20];
	uchar cksum;
	uchar c;
	uchar *p = buf;
	int i;

	/* create the message */
	if(sizeof(buf) < len+4)
		return;
	cksum = (id<<4) | len;
	*p++ = SOF;
	*p++ = cksum;
	for(i = 0; i < len; i++){
		c = data[i];
		cksum += c;
		*p++ = c;
	}
	*p++ = cksum;

	/* send the message - there should be a more generic way to do this */
	serialµcputs(buf, p-buf);
}

/* the tsleep takes care of lost acks */
static void
sendmsgwithack(uchar id, uchar *data, int len)
{
	if(waserror()){
		qunlock(&ctlr);
		nexterror();
	}
	qlock(&ctlr);
	_sendmsg(id, data, len);
	tsleep(&ctlr.r, return0, 0, 100);
	qunlock(&ctlr);
	poperror();
}

static void
sendmsg(uchar id, uchar *data, int len)
{
	if(waserror()){
		qunlock(&ctlr);
		nexterror();
	}
	qlock(&ctlr);
	_sendmsg(id, data, len);
	qunlock(&ctlr);
	poperror();
}

void
µcinit(void)
{
}

static Chan*
µcattach(char* spec)
{
	return devattach('r', spec);
}

static Walkqid*
µcwalk(Chan *c, Chan *nc, char **name, int nname)
{
	return devwalk(c, nc, name, nname, µcdir, nelem(µcdir), devgen);
}

static int	 
µcstat(Chan *c, uchar *dp, int n)
{
	return devstat(c, dp, n, µcdir, nelem(µcdir), devgen);
}

static Chan*
µcopen(Chan* c, int omode)
{
	omode = openmode(omode);
	if(!iseve())
		error(Eperm);
	return devopen(c, omode, µcdir, nelem(µcdir), devgen);
}

static void	 
µcclose(Chan*)
{
}

char*
acstatus(int x)
{
	if(x)
		return "attached";
	else
		return "detached";
}

char*
batstatus(int x)
{
	switch(x){
	case 1:		return "high";
	case 2:		return "low";
	case 4:		return "critical";
	case 8:		return "charging";
	case 0x80:	return "none";
	}
	return "ok";
}

static long	 
µcread(Chan* c, void* a, long n, vlong off)
{
	char buf[64];

	if(c->qid.path == Qdir)
		return devdirread(c, a, n, µcdir, nelem(µcdir), devgen);

	switch((ulong)c->qid.path){
	case Qbattery:
		sendmsgwithack(BLbattery, nil, 0);		/* send a battery request */
		sprint(buf, "voltage: %d\nac: %s\nstatus: %s\n", ctlr.voltage,
			acstatus(ctlr.acstatus),
			batstatus(ctlr.batstatus));
		return readstr(off, a, n, buf);
	case Qversion:
		sendmsgwithack(BLversion, nil, 0);		/* send a battery request */
		return readstr(off, a, n, ctlr.version);
	}
	error(Ebadarg);
	return 0;
}

#define PUTBCD(n,o) bcdclock[o] = (n % 10) | (((n / 10) % 10)<<4)

static uchar lightdata[16];

static long	 
µcwrite(Chan* c, void* a, long n, vlong)
{
	Cmdbuf *cmd;
	uchar data[16];
	char str[64];
	int i, j;
	ulong l;
	Rune r;
	extern ulong resumeaddr[];
	extern void power_resume(void);

	if(c->qid.path == Qkbdin){
		if(n >= sizeof(str))
			n = sizeof(str)-1;
		memmove(str, a, n);
		str[n] = 0;
		for(i = 0; i < n; i += j){
			j = chartorune(&r, &str[i]);
			kbdcr2nl(nil, r);
		}
		return n;
	}
	if(c->qid.path == Qpower){
		if(!iseve())
			error(Eperm);
		if(strncmp(a, "suspend", 7) == 0)
			*resumeaddr = (ulong)power_resume;
		else if(strncmp(a, "halt", 4) == 0)
			*resumeaddr = 0;
		else if(strncmp(a, "wakeup", 6) == 0){
			cmd = parsecmd(a, n);
			if (cmd->nf != 2)
				error(Ebadarg);
			l = strtoul(cmd->f[1], 0, 0);
			rtcalarm(l);
			return n;
		} else
			error(Ebadarg);
		deepsleep();
		return n;
	}

	cmd = parsecmd(a, n);
	if(cmd->nf > 15)
		error(Ebadarg);
	for(i = 0; i < cmd->nf; i++)
		data[i] = atoi(cmd->f[i]);

	switch((ulong)c->qid.path){
	case Qled:
		sendmsgwithack(BLled, data, cmd->nf);
		break;
	case Qbacklight:
		memmove(lightdata, data, 16);
		sendmsgwithack(BLbacklight, data, cmd->nf);
		break;
	case Qcruft:
//		lcdtweak(cmd);
		break;
	default:
		error(Ebadarg);
	}
	return n;
}

void 
µcpower(int on)
{
	uchar data[16];
	if (on == 0)
		return;
	/* maybe dangerous, not holding the lock */
	if (lightdata[0] == 0){
		data[0]= 2;
		data[1]= 1;
		data[2]= 0;
	} else
		memmove(data, lightdata, 16);
	_sendmsg(0xd, data, 3);
	wakeup(&ctlr.r);
}

Dev µcdevtab = {
	'r',
	"µc",

	devreset,
	µcinit,
	devshutdown,
	µcattach,
	µcwalk,
	µcstat,
	µcopen,
	devcreate,
	µcclose,
	µcread,
	devbread,
	µcwrite,
	devbwrite,
	devremove,
	devwstat,
};
