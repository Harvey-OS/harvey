#include <u.h>
#include <libc.h>
#include </386/include/ureg.h>
typedef struct Ureg Ureg;
#include <auth.h>
#include <fcall.h>
#include <thread.h>
#include <9p.h>

enum {
	/* power mgmt event codes */
	NotifyStandbyRequest		= 0x0001,
	NotifySuspendRequest		= 0x0002,
	NotifyNormalResume 		= 0x0003,
	NotifyCriticalResume		= 0x0004,
	NotifyBatteryLow			= 0x0005,
	NotifyPowerStatusChange	= 0x0006,
	NotifyUpdateTime			= 0x0007,
	NotifyCriticalSuspend		= 0x0008,
	NotifyUserStandbyRequest	= 0x0009,
	NotifyUserSuspendRequest	= 0x000A,
	NotifyStandbyResume		= 0x000B,
	NotifyCapabilitiesChange		= 0x000C,

	/* power device ids: add device number or All */
	DevBios					= 0x0000,
	DevAll					= 0x0001,
	DevDisplay				= 0x0100,
	DevStorage				= 0x0200,
	DevLpt					= 0x0300,
	DevEia					= 0x0400,
	DevNetwork				= 0x0500,
	DevPCMCIA				= 0x0600,
	DevBattery				= 0x8000,
		All					= 0x00FF,
	DevMask					= 0xFF00,

	/* power states */
	PowerEnabled				= 0x0000,
	PowerStandby				= 0x0001,
	PowerSuspend				= 0x0002,
	PowerOff					= 0x0003,

	/* apm commands */
	CmdInstallationCheck		= 0x5300,
	CmdRealModeConnect		= 0x5301,
	CmdProtMode16Connect		= 0x5302,
	CmdProtMode32Connect		= 0x5303,
	CmdDisconnect			= 0x5304,
	CmdCpuIdle				= 0x5305,
	CmdCpuBusy				= 0x5306,
	CmdSetPowerState			= 0x5307,
	CmdSetPowerMgmt	= 0x5308,
	  DisablePowerMgmt	= 0x0000,		/* CX */
	  EnablePowerMgmt	= 0x0001,
	CmdRestoreDefaults			= 0x5309,
	CmdGetPowerStatus			= 0x530A,
	CmdGetPMEvent			= 0x530B,
	CmdGetPowerState			= 0x530C,
	CmdGetPowerMgmt			= 0x530D,
	CmdDriverVersion			= 0x530E,

	/* like CmdDisconnect but doesn't lose the interface */
	CmdGagePowerMgmt	= 0x530F,
	  DisengagePowerMgmt	= 0x0000,	/* CX */
	  EngagePowerManagemenet	= 0x0001,

	CmdGetCapabilities			= 0x5310,
	  CapStandby				= 0x0001,
	  CapSuspend				= 0x0002,
	  CapTimerResumeStandby	= 0x0004,
	  CapTimerResumeSuspend	= 0x0008,
	  CapRingResumeStandby		= 0x0010,
	  CapRingResumeSuspend	= 0x0020,
	  CapPcmciaResumeStandby	= 0x0040,
	  CapPcmciaResumeSuspend	= 0x0080,
	  CapSlowCpu				= 0x0100,
	CmdResumeTimer			= 0x5311,
	  DisableResumeTimer		= 0x00,		/* CL */
	  GetResumeTimer			= 0x01,
	  SetResumeTimer			= 0x02,
	CmdResumeOnRing			= 0x5312,
	  DisableResumeOnRing		= 0x0000,		/* CX */
	  EnableResumeOnRing		= 0x0001,
	  GetResumeOnRing			= 0x0002,
	CmdTimerRequests			= 0x5313,
	  DisableTimerRequests		= 0x0000,		/* CX */
	  EnableTimerRequests		= 0x0001,
	  GetTimerRequests			= 0x0002,
};

static char* eventstr[] = {
[NotifyStandbyRequest]	"system standby request",
[NotifySuspendRequest]	"system suspend request",
[NotifyNormalResume]	"normal resume",
[NotifyCriticalResume]	"critical resume",
[NotifyBatteryLow]		"battery low",
[NotifyPowerStatusChange]	"power status change",
[NotifyUpdateTime]		"update time",
[NotifyCriticalSuspend]	"critical suspend",
[NotifyUserStandbyRequest]	"user standby request",
[NotifyUserSuspendRequest]	"user suspend request",
[NotifyCapabilitiesChange]	"capabilities change",
};

static char*
apmevent(int e)
{
	static char buf[32];

	if(0 <= e && e < nelem(eventstr) && eventstr[e])
		return eventstr[e];
	
	sprint(buf, "event 0x%ux", (uint)e);
	return buf;
}

static char *error[256] = {
[0x01]	"power mgmt disabled",
[0x02]	"real mode connection already established",
[0x03]	"interface not connected",
[0x05]	"16-bit protected mode connection already established",
[0x06]	"16-bit protected mode interface not supported",
[0x07]	"32-bit protected mode interface already established",
[0x08]	"32-bit protected mode interface not supported",
[0x09]	"unrecognized device id",
[0x0A]	"parameter value out of range",
[0x0B]	"interface not engaged",
[0x0C]	"function not supported",
[0x0D]	"resume timer disabled",
[0x60]	"unable to enter requested state",
[0x80]	"no power mgmt events pending",
[0x86]	"apm not present",
};

static char*
apmerror(int id)
{
	char *e;
	static char buf[64];

	if(e = error[id&0xFF])
		return e;

	sprint(buf, "unknown error %x", id);
	return buf;
}

QLock apmlock;
int apmdebug;

static int
_apmcall(int fd, Ureg *u)
{
if(apmdebug) fprint(2, "call ax 0x%lux bx 0x%lux cx 0x%lux\n",
	u->ax&0xFFFF, u->bx&0xFFFF, u->cx&0xFFFF);

	seek(fd, 0, 0);
	if(write(fd, u, sizeof *u) != sizeof *u)
		return -1;

	seek(fd, 0, 0);
	if(read(fd, u, sizeof *u) != sizeof *u)
		return -1;

if(apmdebug) fprint(2, "flags 0x%lux ax 0x%lux bx 0x%lux cx 0x%lux\n",
	u->flags&0xFFFF, u->ax&0xFFFF, u->bx&0xFFFF, u->cx&0xFFFF);

	if(u->flags & 1) {	/* carry flag */
		werrstr("%s", apmerror(u->ax>>8));
		return -1;
	}
	return 0;
}

static int
apmcall(int fd, Ureg *u)
{
	int r;

	qlock(&apmlock);
	r = _apmcall(fd, u);
	qunlock(&apmlock);
	return r;
}

typedef struct Apm Apm;
typedef struct Battery Battery;

struct Battery {
	int status;
	int percent;
	int time;
};

enum {
	Mbattery = 4,
};
struct Apm {
	int fd;

	int verhi;
	int verlo;

	int acstatus;
	int nbattery;

	int capabilities;

	Battery battery[Mbattery];
};
enum {
	AcUnknown = 0,		/* Apm.acstatus */
	AcOffline,
	AcOnline,
	AcBackup,

	BatteryUnknown = 0,	/* Battery.status */
	BatteryHigh,
	BatteryLow,
	BatteryCritical,
	BatteryCharging,
};

static char*
acstatusstr[] = {
[AcUnknown]	"unknown",
[AcOffline]	"offline",
[AcOnline]	"online",
[AcBackup]	"backup",
};

static char*
batterystatusstr[] = {
[BatteryUnknown] "unknown",
[BatteryHigh]	"high",
[BatteryLow]	"low",
[BatteryCritical]	"critical",
[BatteryCharging]	"charging",
};

static char*
powerstatestr[] = {
[PowerOff]	"off",
[PowerSuspend]	"suspend",
[PowerStandby]	"standby",
[PowerEnabled]	"on",
};

static char*
xstatus(char **str, int nstr, int x)
{
	if(0 <= x && x < nstr && str[x])
		return str[x];
	return "unknown";
}

static char*
batterystatus(int b)
{
	return xstatus(batterystatusstr, nelem(batterystatusstr), b);
}

static char*
powerstate(int s)
{
	return xstatus(powerstatestr, nelem(powerstatestr), s);
}

static char*
acstatus(int a)
{
	return xstatus(acstatusstr, nelem(acstatusstr), a);
}
	  
static int
apmversion(Apm *apm)
{
	Ureg u;

	u.ax = CmdDriverVersion;
	u.bx = 0x0000;
	u.cx = 0x0102;
	if(apmcall(apm->fd, &u) < 0)
		return -1;

	apm->verhi = u.cx>>8;
	apm->verlo = u.cx & 0xFF;

	return u.cx;
}

static int
apmcpuidle(Apm *apm)
{
	Ureg u;

	u.ax = CmdCpuIdle;
	return apmcall(apm->fd, &u);
}

static int
apmcpubusy(Apm *apm)
{
	Ureg u;

	u.ax = CmdCpuBusy;
	return apmcall(apm->fd, &u);
}

static int
apmsetpowerstate(Apm *apm, int dev, int state)
{
	Ureg u;

	u.ax = CmdSetPowerState;
	u.bx = dev;
	u.cx = state;
	return apmcall(apm->fd, &u);
}

static int
apmsetpowermgmt(Apm *apm, int dev, int state)
{
	Ureg u;

	u.ax = CmdSetPowerMgmt;
	u.bx = dev;
	u.cx = state;
	return apmcall(apm->fd, &u);
}

static int
apmrestoredefaults(Apm *apm, int dev)
{
	Ureg u;

	u.ax = CmdRestoreDefaults;
	u.bx = dev;
	return apmcall(apm->fd, &u);
}

static int
apmgetpowerstatus(Apm *apm, int dev)
{
	Battery *b;
	Ureg u;

	if(dev == DevAll)
		b = &apm->battery[0];
	else if((dev & DevMask) == DevBattery) {
		if(dev - DevBattery < nelem(apm->battery))
			b = &apm->battery[dev - DevBattery];
		else
			b = nil;
	} else {
		werrstr("bad device number");
		return -1;
	}

	u.ax = CmdGetPowerStatus;
	u.bx = dev;

	if(apmcall(apm->fd, &u) < 0)
		return -1;

	if((dev & DevMask) == DevBattery)
		apm->nbattery = u.si;

	switch(u.bx>>8) {
	case 0x00:
		apm->acstatus = AcOffline;
		break;
	case 0x01:
		apm->acstatus = AcOnline;
		break;
	case 0x02:
		apm->acstatus = AcBackup;
		break;
	default:
		apm->acstatus = AcUnknown;
		break;
	}

	if(b != nil) {
		switch(u.bx&0xFF) {
		case 0x00:
			b->status = BatteryHigh;
			break;
		case 0x01:
			b->status = BatteryLow;
			break;
		case 0x02:
			b->status = BatteryCritical;
			break;
		case 0x03:
			b->status = BatteryCharging;
			break;
		default:
			b->status = BatteryUnknown;
			break;
		}

		if((u.cx & 0xFF) == 0xFF)
			b->percent = -1;
		else
			b->percent = u.cx & 0xFF;

		if((u.dx&0xFFFF) == 0xFFFF)
			b->time = -1;
		else if(u.dx & 0x8000)
			b->time = 60*(u.dx & 0x7FFF);
		else
			b->time = u.dx & 0x7FFF;
	}

	return 0;
}

static int
apmgetevent(Apm *apm)
{
	Ureg u;

	u.ax = CmdGetPMEvent;
	u.bx = 0;
	u.cx = 0;

	//when u.bx == NotifyNormalResume or NotifyCriticalResume,
	//u.cx & 1 indicates PCMCIA socket was on while suspended,
	//u.cx & 1 == 0 indicates was off.

	if(apmcall(apm->fd, &u) < 0)
		return -1;

	return u.bx;
}

static int
apmgetpowerstate(Apm *apm, int dev)
{
	Ureg u;

	u.ax = CmdGetPowerState;
	u.bx = dev;
	u.cx = 0;

	if(apmcall(apm->fd, &u) < 0)
		return -1;

	return u.cx;
}

static int
apmgetpowermgmt(Apm *apm, int dev)
{
	Ureg u;

	u.ax = CmdGetPowerMgmt;
	u.bx = dev;

	if(apmcall(apm->fd, &u) < 0)
		return -1;

	return u.cx;
}

static int
apmgetcapabilities(Apm *apm)
{
	Ureg u;

	u.ax = CmdGetCapabilities;
	u.bx = DevBios;

	if(apmcall(apm->fd, &u) < 0)
		return -1;

	apm->nbattery = u.bx & 0xFF;
	apm->capabilities &= ~0xFFFF;
	apm->capabilities |= u.cx;
	return 0;
}

static int
apminstallationcheck(Apm *apm)
{
	Ureg u;

	u.ax = CmdInstallationCheck;
	u.bx = DevBios;
	if(apmcall(apm->fd, &u) < 0)
		return -1;

	if(u.cx & 0x0004)
		apm->capabilities |= CapSlowCpu;
	else
		apm->capabilities &= ~CapSlowCpu;
	return 0;
}

void
apmsetdisplaystate(Apm *apm, int s)
{
	apmsetpowerstate(apm, DevDisplay, s);
}

void
apmblank(Apm *apm)
{
	apmsetdisplaystate(apm, PowerStandby);
}

void
apmunblank(Apm *apm)
{
	apmsetdisplaystate(apm, PowerEnabled);
}

void
apmsuspend(Apm *apm)
{
	apmsetpowerstate(apm, DevAll, PowerSuspend);
}

Apm apm;

void
powerprint(void)
{
	print("%s", ctime(time(0)));
	if(apmgetpowerstatus(&apm, DevAll) == 0) {
		print("%d batteries\n", apm.nbattery);
		print("battery 0: status %s percent %d time %d:%.2d\n",
			batterystatus(apm.battery[0].status), apm.battery[0].percent,
			apm.battery[0].time/60, apm.battery[0].time%60);
	}
}

void*
erealloc(void *v, ulong n)
{
	v = realloc(v, n);
	if(v == nil)
		sysfatal("out of memory reallocating %lud", n);
	setmalloctag(v, getcallerpc(&v));
	return v;
}

void*
emalloc(ulong n)
{
	void *v;

	v = malloc(n);
	if(v == nil)
		sysfatal("out of memory allocating %lud", n);
	memset(v, 0, n);
	setmalloctag(v, getcallerpc(&n));
	return v;
}

char*
estrdup(char *s)
{
	int l;
	char *t;

	if (s == nil)
		return nil;
	l = strlen(s)+1;
	t = emalloc(l);
	memcpy(t, s, l);
	setmalloctag(t, getcallerpc(&s));
	return t;
}

char*
estrdupn(char *s, int n)
{
	int l;
	char *t;

	l = strlen(s);
	if(l > n)
		l = n;
	t = emalloc(l+1);
	memmove(t, s, l);
	t[l] = '\0';
	setmalloctag(t, getcallerpc(&s));
	return t;
}

enum {
	Qroot = 0,
	Qevent,
	Qbattery,
	Qctl,
};

static void rootread(Req*);
static void eventread(Req*);
static void ctlread(Req*);
static void ctlwrite(Req*);
static void batteryread(Req*);

typedef struct Dfile Dfile;
struct Dfile {
	Qid qid;
	char *name;
	ulong mode;
	void (*read)(Req*);
	void (*write)(Req*);
};

Dfile dfile[] = {
	{ {Qroot,0,QTDIR},		"/",		DMDIR|0555,	rootread,		nil, },
	{ {Qevent},		"event",	0444,		eventread,	nil, },
	{ {Qbattery},	"battery",	0444,		batteryread,	nil, },
	{ {Qctl},		"ctl",		0666,		ctlread,		ctlwrite, },
};

static int
fillstat(uvlong path, Dir *d, int doalloc)
{
	int i;

	for(i=0; i<nelem(dfile); i++)
		if(path==dfile[i].qid.path)
			break;
	if(i==nelem(dfile))
		return -1;

	memset(d, 0, sizeof *d);
	d->uid = doalloc ? estrdup("apm") : "apm";
	d->gid = doalloc ? estrdup("apm") : "apm";
	d->length = 0;
	d->name = doalloc ? estrdup(dfile[i].name) : dfile[i].name;
	d->mode = dfile[i].mode;
	d->atime = d->mtime = time(0);
	d->qid = dfile[i].qid;
	return 0;
}

static char*
fswalk1(Fid *fid, char *name, Qid *qid)
{
	int i;

	if(strcmp(name, "..")==0){
		*qid = dfile[0].qid;
		fid->qid = *qid;
		return nil;
	}

	for(i=1; i<nelem(dfile); i++){	/* i=1: 0 is root dir */
		if(strcmp(dfile[i].name, name)==0){
			*qid = dfile[i].qid;
			fid->qid = *qid;
			return nil;
		}
	}
	return "file does not exist";
}

static void
fsopen(Req *r)
{
	switch((ulong)r->fid->qid.path){
	case Qroot:
		r->fid->aux = (void*)0;
		respond(r, nil);
		return;

	case Qevent:
	case Qbattery:
		if(r->ifcall.mode == OREAD){
			respond(r, nil);
			return;
		}
		break;

	case Qctl:
		if((r->ifcall.mode&~(OTRUNC|OREAD|OWRITE|ORDWR)) == 0){
			respond(r, nil);
			return;
		}
		break;
	}
	respond(r, "permission denied");
	return;
}

static void
fsstat(Req *r)
{
	fillstat(r->fid->qid.path, &r->d, 1);
	respond(r, nil);
}

static void
fsread(Req *r)
{
	dfile[r->fid->qid.path].read(r);
}

static void
fswrite(Req *r)
{
	dfile[r->fid->qid.path].write(r);
}

static void
rootread(Req *r)
{
	int n;
	uvlong offset;
	char *p, *ep;
	Dir d;

	if(r->ifcall.offset == 0)
		offset = 0;
	else
		offset = (uvlong)r->fid->aux;

	p = r->ofcall.data;
	ep = r->ofcall.data+r->ifcall.count;

	if(offset == 0)		/* skip root */
		offset = 1;
	for(; p+2 < ep; p+=n){
		if(fillstat(offset, &d, 0) < 0)
			break;
		n = convD2M(&d, (uchar*)p, ep-p);
		if(n <= BIT16SZ)
			break;
		offset++;
	}
	r->fid->aux = (void*)offset;
	r->ofcall.count = p - r->ofcall.data;
	respond(r, nil);
}

static void
batteryread(Req *r)
{
	char buf[Mbattery*80], *ep, *p;
	int i;

	apmgetpowerstatus(&apm, DevAll);

	p = buf;
	ep = buf+sizeof buf;
	*p = '\0';	/* could be no batteries */
	for(i=0; i<apm.nbattery && i<Mbattery; i++)
		p += snprint(p, ep-p, "%s %d %d\n",
			batterystatus(apm.battery[i].status),
			apm.battery[i].percent, apm.battery[i].time);
	
	readstr(r, buf);
	respond(r, nil);
}

int
iscmd(char *p, char *cmd)
{
	int l;

	l = strlen(cmd);
	return strncmp(p, cmd, l)==0 && p[l]=='\0' || p[l]==' ' || p[l]=='\t';
}

char*
skip(char *p, char *cmd)
{
	p += strlen(cmd);
	while(*p==' ' || *p=='\t')
		p++;
	return p;
}

static void
respondx(Req *r, int c)
{
	char err[ERRMAX];

	if(c == 0)
		respond(r, nil);
	else{
		rerrstr(err, sizeof err);
		respond(r, err);
	}
}

/*
 * we don't do suspend because it messes up the
 * cycle counter as well as the pcmcia ethernet cards.
 */
static void
ctlwrite(Req *r)
{
	char buf[80], *p, *q;
	int dev;
	long count;

	count = r->ifcall.count;
	if(count > sizeof(buf)-1)
		count = sizeof(buf)-1;
	memmove(buf, r->ifcall.data, count);
	buf[count] = '\0';

	if(count && buf[count-1] == '\n'){
		--count;
		buf[count] = '\0';
	}

	q = buf;
	p = strchr(q, ' ');
	if(p==nil)
		p = q+strlen(q);
	else
		*p++ = '\0';

	if(strcmp(q, "")==0 || strcmp(q, "system")==0)
		dev = DevAll;
	else if(strcmp(q, "display")==0)
		dev = DevDisplay;
	else if(strcmp(q, "storage")==0)
		dev = DevStorage;
	else if(strcmp(q, "lpt")==0)
		dev = DevLpt;
	else if(strcmp(q, "eia")==0)
		dev = DevEia;
	else if(strcmp(q, "network")==0)
		dev = DevNetwork;
	else if(strcmp(q, "pcmcia")==0)
		dev = DevPCMCIA;
	else{
		respond(r, "unknown device");
		return;
	}

	if(strcmp(p, "enable")==0)
		respondx(r, apmsetpowermgmt(&apm, dev, EnablePowerMgmt));
	else if(strcmp(p, "disable")==0)
		respondx(r, apmsetpowermgmt(&apm, dev, DisablePowerMgmt));
	else if(strcmp(p, "standby")==0)
		respondx(r, apmsetpowerstate(&apm, dev, PowerStandby));
	else if(strcmp(p, "on")==0)
		respondx(r, apmsetpowerstate(&apm, dev, PowerEnabled));
/*
	else if(strcmp(p, "off")==0)
		respondx(r, apmsetpowerstate(&apm, dev, PowerOff));
*/
	else if(strcmp(p, "suspend")==0)
		respondx(r, apmsetpowerstate(&apm, dev, PowerSuspend));
	else
		respond(r, "unknown verb");
}

static int
statusline(char *buf, int nbuf, char *name, int dev)
{
	int s;
	char *state;

	state = "unknown";
	if((s = apmgetpowerstate(&apm, dev)) >= 0)
		state = powerstate(s);
	return snprint(buf, nbuf, "%s %s\n", name, state);
}

static void
ctlread(Req *r)
{
	char buf[256+7*50], *ep, *p;

	p = buf;
	ep = buf+sizeof buf;

	p += snprint(p, ep-p, "ac %s\n", acstatus(apm.acstatus));
	p += snprint(p, ep-p, "capabilities");
	if(apm.capabilities & CapStandby) 
		p += snprint(p, ep-p, " standby");
	if(apm.capabilities & CapSuspend) 
		p += snprint(p, ep-p, " suspend");
	if(apm.capabilities & CapSlowCpu)
		p += snprint(p, ep-p, " slowcpu");
	p += snprint(p, ep-p, "\n");

	p += statusline(p, ep-p, "system", DevAll);
	p += statusline(p, ep-p, "display", DevDisplay);
	p += statusline(p, ep-p, "storage", DevStorage);
	p += statusline(p, ep-p, "lpt", DevLpt);
	p += statusline(p, ep-p, "eia", DevEia|All);
	p += statusline(p, ep-p, "network", DevNetwork|All);
	p += statusline(p, ep-p, "pcmcia", DevPCMCIA|All);
	USED(p);

	readstr(r, buf);
	respond(r, nil);
}

enum {
	STACK = 16384,
};

Channel *creq;
Channel *cflush;
Channel *cevent;
Req *rlist, **tailp;
int rp, wp;
int nopoll;
char eventq[32][80];

static void
flushthread(void*)
{
	Req *r, *or, **rq;

	threadsetname("flushthread");
	while(r = recvp(cflush)){
		or = r->oldreq;
		for(rq=&rlist; *rq; rq=&(*rq)->aux){
			if(*rq == or){
				*rq = or->aux;
				if(tailp==&or->aux)
					tailp = rq;
				respond(or, "interrupted");
				break;
			}
		}
		respond(r, nil);
	}
}

static void
answerany(void)
{
	char *buf;
	int l, m;
	Req *r;

	if(rlist==nil || rp==wp)
		return;

	while(rlist && rp != wp){
		r = rlist;
		rlist = r->aux;
		if(rlist==nil)
			tailp = &rlist;

		l = 0;
		buf = r->ofcall.data;
		m = r->ifcall.count;
		while(rp != wp){
			if(l+strlen(eventq[rp]) <= m){
				strcpy(buf+l, eventq[rp]);
				l += strlen(buf+l);
			}else if(l==0){
				strncpy(buf, eventq[rp], m-1);
				buf[m-1] = '\0';
				l += m;
			}else
				break;
			rp++;
			if(rp == nelem(eventq))
				rp = 0;
		}
		r->ofcall.count = l;
		respond(r, nil);
	}
}

static void
eventwatch(void*)
{
	int e, s;

	threadsetname("eventwatch");
	for(;;){
		s = 0;
		while((e = apmgetevent(&apm)) >= 0){
			sendul(cevent, e);
			s = 1;
		}
		if(s)
			sendul(cevent, -1);
		if(sleep(750) < 0)
			break;
	}
}

static void
eventthread(void*)
{
	int e;

	threadsetname("eventthread");
	for(;;){
		while((e = recvul(cevent)) >= 0){
			snprint(eventq[wp], sizeof(eventq[wp])-1, "%s", apmevent(e));
			strcat(eventq[wp], "\n");
			wp++;
			if(wp==nelem(eventq))
				wp = 0;
			if(wp+1==rp || (wp+1==nelem(eventq) && rp==0))
				break;
		}
		answerany();
	}
}

static void
eventproc(void*)
{
	Req *r;

	threadsetname("eventproc");

	creq = chancreate(sizeof(Req*), 0);
	cevent = chancreate(sizeof(ulong), 0);
	cflush = chancreate(sizeof(Req*), 0);

	tailp = &rlist;
	if(!nopoll)
		proccreate(eventwatch, nil, STACK);
	threadcreate(eventthread, nil, STACK);
	threadcreate(flushthread, nil, STACK);

	while(r = recvp(creq)){
		*tailp = r;
		r->aux = nil;
		tailp = &r->aux;
		answerany();
	}
}

static void
fsflush(Req *r)
{
	sendp(cflush, r);
}

static void
eventread(Req *r)
{
	sendp(creq, r);
}

static void
fsattach(Req *r)
{
	char *spec;
	static int first = 1;

	spec = r->ifcall.aname;

	if(first){
		first = 0;
		proccreate(eventproc, nil, STACK);
	}

	if(spec && spec[0]){
		respond(r, "invalid attach specifier");
		return;
	}
	r->fid->qid = dfile[0].qid;
	r->ofcall.qid = dfile[0].qid;
	respond(r, nil);
}

Srv fs = {
.attach=	fsattach,
.walk1=	fswalk1,
.open=	fsopen,
.read=	fsread,
.write=	fswrite,
.stat=	fsstat,
.flush=	fsflush,
};

void
usage(void)
{
	fprint(2, "usage: aux/apm [-ADPi] [-d /dev/apm] [-m /mnt/apm] [-s service]\n");
	exits("usage");
}

void
threadmain(int argc, char **argv)
{
	char *dev, *mtpt, *srv;

	dev = nil;
	mtpt = "/mnt/apm";
	srv = nil;
	ARGBEGIN{
	case 'A':
		apmdebug = 1;
		break;
	case 'D':
		chatty9p = 1;
		break;
	case 'P':
		nopoll = 1;
		break;
	case 'd':
		dev = EARGF(usage());
		break;
	case 'i':
		fs.nopipe++;
		fs.infd = 0;
		fs.outfd = 1;
		mtpt = nil;
		break;
	case 'm':
		mtpt = EARGF(usage());
		break;
	case 's':
		srv = EARGF(usage());
		break;
	}ARGEND

	if(dev == nil){
		if((apm.fd = open("/dev/apm", ORDWR)) < 0
		&& (apm.fd = open("#P/apm", ORDWR)) < 0){
			fprint(2, "open %s: %r\n", dev);
			threadexitsall("open");
		}
	} else if((apm.fd = open(dev, ORDWR)) < 0){
		fprint(2, "open %s: %r\n", dev);
		threadexitsall("open");
	}

	if(apmversion(&apm) < 0){
		fprint(2, "cannot get apm version: %r\n");
		threadexitsall("apmversion");
	}

	if(apm.verhi < 1 || (apm.verhi==1 && apm.verlo < 2)){
		fprint(2, "apm version %d.%d not supported\n", apm.verhi, apm.verlo);
		threadexitsall("apmversion");
	}

	if(apmgetcapabilities(&apm) < 0)
		fprint(2, "warning: cannot read apm capabilities: %r\n");

	apminstallationcheck(&apm);
	apmcpuidle(&apm);

	rfork(RFNOTEG);
	threadpostmountsrv(&fs, srv, mtpt, MREPL);
}
