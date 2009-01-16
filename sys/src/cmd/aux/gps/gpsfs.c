#include <u.h>
#include <libc.h>
#include <fcall.h>
#include <thread.h>
#include <ctype.h>
#include <9p.h>
#include "dat.h"

enum
{
	Numsize=	12,
	Vlnumsize=	22,
	Rawbuf=		0x10000,
	Rawmask=	Rawbuf-1,
};

#define	nsecperchar	((int)(1000000000.0 * 10.0 / baud))

typedef struct Fix Fix;
typedef struct Satellite Satellite;
typedef struct GPSfile GPSfile;
typedef struct Gpsmsg Gpsmsg;

struct Satellite {
	int		prn;
	int		elevation;
	int		azimuth;
	int		snr;
};

struct Fix {
	int		messages;	/* bitmap of types seen */
	Place;
	/*
	 * The following are in Plan 9 time format:
	 * seconds or nanoseconds since the epoch.
	 */
	vlong		localtime;	/* nsec() value when first byte was read */
	vlong		gpstime;	/* nsec() value from GPS */
	long		time;		/* time() value from GPS */

	double		zulu;
	int		date;
	char		valid;
	uchar		quality;
	ushort		satellites;
	double		pdop;
	double		hdop;
	double		vdop;
	double		altitude;
	double		sealevel;
	double		groundspeed;
	double		kmh;
	double		course;
	double		heading;
	double		magvar;
	Satellite	s[12];
};

struct GPSfile {
	char	*name;
	char*	(*rread)(Req*);
	int	mode;
	vlong	offset;		/* for raw: rawout - read-offset */
};

enum {
	ASTRAL,
	GPGGA,
	GPGLL,
	GPGSA,
	GPGSV,
	GPRMC,
	GPVTG,
	PRWIRID,
	PRWIZCH
};

struct Gpsmsg {
	char *name;
	int tokens;
	ulong errors;
};

char	raw[Rawbuf];
vlong	rawin;
vlong	rawout;

ulong	badlat, goodlat, suspectlat;
ulong	badlon, goodlon, suspectlon;
ulong	suspecttime, goodtime;

ulong histo[32];

char *serial = "/dev/eia0";

Gpsmsg gpsmsg[] = {
[ASTRAL]	= { "ASTRAL",	 0,	0},
[GPGGA]		= { "$GPGGA",	15,	0},
/* NMEA 2.3 permits optional 8th field, mode */
[GPGLL]		= { "$GPGLL",	 7,	0},
[GPGSA]		= { "$GPGSA",	18,	0},
[GPGSV]		= { "$GPGSV",	0,	0},
[GPRMC]		= { "$GPRMC",	0,	0},
[GPVTG]		= { "$GPVTG",	0,	0},
[PRWIRID]	= { "$PRWIRID",	0,	0},
[PRWIZCH]	= { "$PRWIZCH",	0,	0},
};

int ttyfd, ctlfd, debug;
int setrtc;
int baud = Baud;
char *baudstr = "b%dd1r1pns1l8i9";
ulong seconds;
ulong starttime;
ulong checksumerrors;
int gpsplayback;	/* If set, return times and positions with `invalid' marker set */

Place where = {-(74.0 + 23.9191/60.0), 40.0 + 41.1346/60.0};

Fix curfix;
Lock fixlock;

int	type(char*);
void	setline(void);
int	getonechar(vlong*);
void	getline(char*, int, vlong*);
void	putline(char*);
int	gettime(Fix*);
int	getzulu(char *, Fix*);
int	getalt(char*, char*, Fix*);
int	getsea(char*, char*, Fix*);
int	getlat(char*, char*, Fix*);
int	getlon(char*, char*, Fix*);
int	getgs(char*, Fix *);
int	getkmh(char*, Fix*);
int	getcrs(char*, Fix*);
int	gethdg(char*, Fix*);
int	getdate(char*, Fix*);
int	getmagvar(char*, char*, Fix*);
void	printfix(int, Fix*);
void	ropen(Req *r);
void	rread(Req *r);
void	rend(Srv *s);
void	gpsinit(void);
char*	readposn(Req*);
char*	readtime(Req*);
char*	readsats(Req*);
char*	readstats(Req*);
char*	readraw(Req*);

GPSfile files[] = {
	{ "time",	readtime,	0444,	0 },
	{ "position",	readposn,	0444,	0 },
	{ "satellites",	readsats,	0444,	0 },
	{ "stats",	readstats,	0444,	0 },
	{ "raw",	readraw,	DMEXCL|0444, 0 },
};

Srv s = {
	.open	= ropen,
	.read	= rread,

	.end = rend,
};

File *root;
File *gpsdir;

void
rend(Srv *)
{
	sysfatal("gpsfs demised");
}

void
ropen(Req *r)
{
	respond(r, nil);
}

void
rread(Req *r)
{
	GPSfile *f;

	r->ofcall.count = 0;
	f = r->fid->file->aux;
	respond(r, f->rread(r));
}

void
fsinit(void)
{
	char* user;
	int i;

	user = getuser();
	s.tree = alloctree(user, user, 0555, nil);
	if(s.tree == nil)
		sysfatal("fsinit: alloctree: %r");
	root = s.tree->root;
	if((gpsdir = createfile(root, "gps", user, DMDIR|0555, nil)) == nil)
		sysfatal("fsinit: createfile: gps: %r");
	for(i = 0; i < nelem(files); i++)
		if(createfile(gpsdir, files[i].name, user, files[i].mode, files + i) == nil)
			sysfatal("fsinit: createfile: %s: %r", files[i].name);
}

void
threadmain(int argc, char*argv[])
{
	char *srvname, *mntpt;

	srvname = "gps";
	mntpt = "/mnt";

	ARGBEGIN {
	default:
		fprint(2, "usage: %s [-b baud] [-d device] [-l logfile] [-m mntpt] [-r] [-s postname]\n", argv0);
		exits("usage");
	case 'D':
		debug++;
		break;
	case 'b':
		baud = strtol(ARGF(), nil, 0);
		break;
	case 'd':
		serial = ARGF();
		break;
	case 'r':
		setrtc = 1;
		break;
	case 's':
		srvname = ARGF();
		break;
	case 'm':
		mntpt = ARGF();
		break;
	} ARGEND

	fmtinstall('L', placeconv);
	
	rfork(RFNOTEG);

	fsinit();
	gpsinit();
	threadpostmountsrv(&s, srvname, mntpt, MBEFORE);
	threadexits(nil);
}

static void
gpstrack(void *)
{
	Fix fix;
	static char buf[256], *t[32];
	int n, i, k, tp;
	vlong localtime;
	double d;

	setline();
	fix.messages = 0;
	fix.lon = 181.0;
	fix.lat = 91.0;
	fix.zulu = 0;
	fix.date = 0;
	fix.valid = 0;
	fix.quality = 0;
	fix.satellites = 0;
	fix.pdop = 0.0;
	fix.hdop = 0.0;
	fix.vdop = 0.0;
	fix.altitude = 0.0;
	fix.sealevel = 0.0;
	fix.groundspeed = 0.0;
	fix.kmh = 0.0;
	fix.course = 0.0;
	fix.heading = 0.0;
	fix.magvar = 0.0;
	for(;;){
		getline(buf, sizeof buf, &localtime);
		n = getfields(buf, t, nelem(t), 0,",\r\n");
		if(n == 0)
			continue;
		tp = type(t[0]);
		if(tp >= 0 && tp < nelem(gpsmsg) && gpsmsg[tp].tokens &&
		    gpsmsg[tp].tokens > n){
			gpsmsg[tp].errors++;
			if(debug)
				fprint(2, "%s: Expect %d tokens, got %d\n",
					gpsmsg[tp].name, gpsmsg[tp].tokens, n);
			continue;
		}
		switch(tp){
		case ASTRAL:
			putline("$IIGPQ,ASTRAL*73");
			putline("$PRWIILOG,GGA,A,T,10,0");
			putline("$PRWIILOG,RMC,A,T,10,0");
			putline("$PRWIILOG,GSA,A,T,10,0");
			putline("$PRWIILOG,GSV,V,,,");
			fprint(2, "Reply: %s\n", "$IIGPQ,ASTRAL*73");
			break;
		case PRWIRID:
		case PRWIZCH:
			for(i = 0; i < n; i++) fprint(2, "%s,", t[i]);
			fprint(2, "(%d tokens)\n", n);
			break;
		case GPGGA:
			if(getlat(t[2], t[3], &fix))
				break;
			if(getlon(t[4], t[5], &fix))
				break;
			getzulu(t[1], &fix);
			if(fix.date && gettime(&fix))
				break;
			if(isdigit(*t[7]))
				fix.satellites = strtol(t[7], nil, 10);
			if(isdigit(*t[8])){
				d = strtod(t[8], nil);
				if(!isNaN(d))
					fix.hdop = d;
			}
			getalt(t[9], t[10], &fix);
			getsea(t[11], t[12], &fix);
			fix.localtime = localtime;
			fix.quality = strtol(t[6], nil, 10);
			fix.messages |= 1 << tp;
			break;
		case GPRMC:
			fix.valid = *t[2];
			getgs(t[7], &fix);
			getcrs(t[8], &fix);
			getdate(t[9], &fix);
			getmagvar(t[10], t[11], &fix);
			if((fix.messages & (1 << GPGGA)) == 0){
				if(getlat(t[3], t[4], &fix))
					break;
				if(getlon(t[5], t[6], &fix))
					break;
				fix.localtime = localtime;
				getzulu(t[1], &fix);
				if(fix.date)
					gettime(&fix);
			}
			fix.messages |= 1 << tp;
			break;
		case GPGSA:
			if(*t[15]){
				d = strtod(t[15], nil);
				if(!isNaN(d))
					fix.pdop = d;
			}
			if(*t[16]){
				d = strtod(t[16], nil);
				if(!isNaN(d))
					fix.hdop = d;
			}
			if(*t[17]){
				d = strtod(t[17], nil);
				if(!isNaN(d))
					fix.vdop = d;
			}
			fix.messages |= 1 << tp;
			break;
		case GPGLL:
			if(getlat(t[1], t[2], &fix))
				break;
			if(getlon(t[3], t[4], &fix))
				break;
			getzulu(t[5], &fix);
			fix.messages |= 1 << tp;
			break;
		case GPGSV:
			if(n < 8){
				gpsmsg[tp].errors++;
				if(debug)
					fprint(2, "%s: Expect at least 8 tokens, got %d\n",
						gpsmsg[tp].name, n);
				break;
			}
			i = 4*(strtol(t[2], nil, 10)-1);	/* starting entry in satellite table */
			fix.satellites = strtol(t[3], nil, 10);
			k = 4;
			while(i < nelem(fix.s) && k + 3 < n){
				fix.s[i].prn = strtol(t[k++], nil, 10);
				fix.s[i].elevation = strtol(t[k++], nil, 10);
				fix.s[i].azimuth = strtol(t[k++], nil, 10);
				fix.s[i].snr = strtol(t[k++], nil, 10);
				k += 4;
				i++;
			} 
			fix.messages |= 1 << tp;
			break;
		case GPVTG:
			if(n < 8){
				gpsmsg[tp].errors++;
				if(debug)
					fprint(2, "%s: Expect at least 8 tokens, got %d\n",
						gpsmsg[tp].name, n);
				break;
			}
			getcrs(t[2], &fix);
			gethdg(t[4], &fix);
			getgs(t[6], &fix);
			if(n > 8)
				getkmh(t[8], &fix);
			fix.messages |= 1 << tp;
			break;
		default:
			if(debug && fix.date)
				fprint(2, "Don't know %s\n", t[0]);
			break;
		}
		if(fix.valid){
			seconds++;
			lock(&fixlock);
			memmove(&curfix, &fix, sizeof fix);
			unlock(&fixlock);
			if(debug)
				printfix(2, &fix);
			fix.valid = 0;
			fix.messages = 0;
			for(i = 0; i < nelem(fix.s); i++)
				fix.s[i].prn = 0;
			if(gpsplayback)
				sleep(100);
		}
	}
}

void
gpsinit(void)
{
	proccreate(gpstrack, nil, 4096);
}

void
printfix(int f, Fix *fix){
	int i;

	fprint(f, "%L, ", fix->Place);
	fprint(f, "%g, ", fix->magvar);
	fprint(f, "%gm - %gm = %gm, ", fix->altitude, fix->sealevel, fix->altitude - fix->sealevel);
	fprint(f, "%06dZ(%g)-", (int)fix->zulu, fix->zulu);
	fprint(f, "%06d\n", fix->date);
	if(fix->lat >= 0)
		fprint(f, "%11.8fN, ", fix->lat);
	else
		fprint(f, "%11.8fS, ", -fix->lat);		
	if(fix->lon >= 0)
		fprint(f, "%12.8fE, ", fix->lon);
	else
		fprint(f, "%12.8fW, ", -fix->lon);
	fprint(f, "%g@%g, ", fix->course, fix->groundspeed);
	fprint(f, "(%c, %ds)\n", fix->valid, fix->satellites);
	for(i = 0; i < nelem(fix->s); i++){
		if(fix->s[i].prn == 0)
			continue;
		fprint(f, "[%d, %d°, %d°, %d]\n",
			fix->s[i].prn, fix->s[i].elevation, fix->s[i].azimuth, fix->s[i].snr);
	}
}

char*
readposn(Req *r)
{
	Fix f;
	char buf[256];

	lock(&fixlock);
	memmove(&f, &curfix, sizeof f);
	unlock(&fixlock);
	snprint(buf, sizeof buf, "%x	%06dZ	%lud	%g	%g	%g	%g	%g	%g",
		gpsplayback|f.quality, (int)f.zulu, f.time, f.lon, f.lat, f.altitude - f.sealevel,
		f.course, f.groundspeed, f.magvar);
	readstr(r, buf);
	return nil;
}

char*
readtime(Req *r)
{
	Fix f;
	char buf[Numsize+Vlnumsize+Vlnumsize+8];

	lock(&fixlock);
	memmove(&f, &curfix, sizeof f);
	unlock(&fixlock);
	seprint(buf, buf + sizeof buf, "%*.0lud %*.0llud %*.0llud %c",
		Numsize-1, f.time,
		Vlnumsize-1, f.gpstime,
		Vlnumsize-1, f.localtime, f.valid + (gpsplayback?1:0));
	readstr(r, buf);
	return nil;
}

char*
readstats(Req *r)
{
	int i;
	char buf[1024], *p;

	p = buf;
	p = seprint(p, buf + sizeof buf, "%lld bytes read, %ld samples processed in %ld seconds\n",
		rawin, seconds, curfix.time - starttime);
	p = seprint(p, buf + sizeof buf, "%lud checksum errors\n", checksumerrors);
	p = seprint(p, buf + sizeof buf, "format errors:");
	for(i = 0; i < nelem(gpsmsg); i++){
		p = seprint(p, buf + sizeof buf, "[%s]: %ld, ",
			gpsmsg[i].name, gpsmsg[i].errors);
	}
	p = seprint(p, buf + sizeof buf, "\nhistogram of # bytes received per buffer:\n");
	for(i = 0; i < nelem(histo); i++){
		p = seprint(p, buf + sizeof buf, "[%d]: %ld ",
			i, histo[i]);
	}
	p = seprint(p, buf + sizeof buf, "\n");
	p = seprint(p, buf + sizeof buf, "bad/good/suspect lat: %lud/%lud/%lud\n",
		badlat, goodlat, suspectlat);
	p = seprint(p, buf + sizeof buf, "bad/good/suspect lon: %lud/%lud/%lud\n",
		badlon, goodlon, suspectlon);
	p = seprint(p, buf + sizeof buf, "good/suspect time: %lud/%lud\n", goodtime, suspecttime);
	USED(p);
	readstr(r, buf);
	return nil;
}

char*
readsats(Req *r)
{
	Fix f;
	int i;
	char buf[1024], *p;

	lock(&fixlock);
	memmove(&f, &curfix, sizeof f);
	unlock(&fixlock);
	p = seprint(buf, buf + sizeof buf, "%d	%d\n", gpsplayback|f.quality, f.satellites);
	for(i = 0; i < nelem(f.s); i++){
		if(f.s[i].prn == 0)
			continue;
		p = seprint(p, buf + sizeof buf, "%d	%d	%d	%d\n",
			f.s[i].prn, f.s[i].elevation, f.s[i].azimuth, f.s[i].snr);
	}
	readstr(r, buf);
	return nil;
}

char*
readraw(Req *r)
{
	int n;
	GPSfile *f;

	f = r->fid->file->aux;
	if(rawin - rawout > Rawbuf){
		rawout = rawin - Rawbuf;
		f->offset = rawout - r->ifcall.offset;
	}
	n = Rawbuf - (rawout&Rawmask);
	if(rawin - rawout < n)
		n = rawin - rawout;
	if(r->ifcall.count < n)
		n = r->ifcall.count;
	r->ofcall.count = n;
	if(n > 0){
		memmove(r->ofcall.data, raw + (rawout & Rawmask), n);
		rawout += n;
	}
	return nil;
}

void
rtcset(long t)
{
	static int fd;
	long r;
	int n;
	char buf[32];

	if(fd <= 0 && (fd = open("#r/rtc", ORDWR)) < 0){
		fprint(2, "Can't open #r/rtc: %r\n");
		return;
	}
	n = read(fd, buf, sizeof buf - 1);
	if(n <= 0){
		fprint(2, "Can't read #r/rtc: %r\n");
		return;
	}
	buf[n] = '\0';
	r = strtol(buf, nil, 0);
	if(r <= 0){
		fprint(2, "ridiculous #r/rtc: %ld\n", r);
		return;
	}
	if(r - t > 1 || t - r > 0){
		seek(fd, 0, 0);
		fprint(fd, "%ld", t);
		fprint(2, "correcting #r/rtc: %ld → %ld\n", r, t);
	}
	seek(fd, 0, 0);
}

int
gettime(Fix *f){
	/* Convert zulu time and date to Plan9 time(2) */
	Tm tm;
	int zulu;
	double d;
	long t;
	static int count;

	zulu = f->zulu;
	memset(&tm, 0, sizeof tm );
	tm.sec = zulu % 100;
	tm.min = (zulu/100) % 100;
	tm.hour = zulu / 10000;
	tm.year = f->date % 100 + 100;	/* This'll only work until 2099 */
	tm.mon = ((f->date/100) % 100) - 1;
	tm.mday = f->date / 10000;
	strcpy(tm.zone, "GMT");
	t = tm2sec(&tm);
	if(f->time && count < 3 && (t - f->time > 10 || t - f->time <= 0)){
		count++;
		suspecttime++;
		return -1;
	}
	goodtime++;
	f->time = t;
	count = 0;
	if(starttime == 0) starttime = t;
	f->gpstime = 1000000000LL * t + 1000000 * (int)modf(f->zulu, &d);
	if(setrtc){
		if(setrtc == 1 || (t % 300) == 0){
			rtcset(t);
			setrtc++;
		}
	}
	return 0;
}

int
getzulu(char *s, Fix *f){
	double d;

	if(*s == '\0') return 0;
	if(isdigit(*s)){
		d = strtod(s, nil);
		if(!isNaN(d))
			f->zulu = d;
		return 1;
	}
	return 0;
}

int
getdate(char *s, Fix *f){
	if(*s == 0) return 0;
	if(isdigit(*s)){
		f->date = strtol(s, nil, 10);
		return 1;
	}
	return 0;
}

int
getgs(char *s, Fix *f){
	double d;

	if(*s == 0) return 0;
	if(isdigit(*s)){
		d = strtod(s, nil);
		if(!isNaN(d))
			f->groundspeed = d;
		return 1;
	}
	return 0;
}

int
getkmh(char *s, Fix *f){
	double d;

	if(*s == 0) return 0;
	if(isdigit(*s)){
		d = strtod(s, nil);
		if(!isNaN(d))
			f->kmh = d;
		return 1;
	}
	return 0;
}

int
getcrs(char *s1, Fix *f){
	double d;

	if(*s1 == 0) return 0;
	if(isdigit(*s1)){
		d = strtod(s1, nil);
		if(!isNaN(d))
			f->course = d;
		return 1;
	}
	return 0;
}

int
gethdg(char *s1, Fix *f){
	double d;

	if(*s1 == 0) return 0;
	if(isdigit(*s1)){
		d = strtod(s1, nil);
		if(!isNaN(d))
			f->heading = d;
		return 1;
	}
	return 0;
}

int
getalt(char *s1, char *s2, Fix *f){
	double alt;

	if(*s1 == 0) return 0;
	if(isdigit(*s1)){
		alt = strtod(s1, nil);
		if(*s2 == 'M' && !isNaN(alt)){
			f->altitude = alt;
			return 1;
		}
		return 0;
	}
	return 0;
}

int
getsea(char *s1, char *s2, Fix *f){
	double alt;

	if(*s1 == 0) return 0;
	if(isdigit(*s1)){
		alt = strtod(s1, nil);
		if(*s2 == 'M'){
			f->sealevel = alt;
			return 1;
		}
		return 0;
	}
	return 0;
}

int
getlat(char *s1, char *s2, Fix *f){
	double lat;
	static count;

	if(*s1 == 0 || !isdigit(*s1) || strlen(s1) <= 5){
		badlat++;
		return -1;
	}
	lat = strtod(s1+2, nil);
	if(isNaN(lat)){
		badlat++;
		return -1;
	}
	lat /= 60.0;
	lat += 10*(s1[0] - '0') + s1[1] - '0';
	if(lat < 0 || lat > 90.0){
		badlat++;
		return -1;
	}
	switch(*s2){
	default:
		badlat++;
		return -1;
	case 'S':
		lat = -lat;
	case 'N':
		break;
	}
	if(f->lat <= 90.0 && count < 3 && fabs(f->lat - lat) > 10.0){
		count++;
		suspectlat++;
		return -1;
	}
	f->lat = lat;
	count = 0;
	goodlat++;
	return 0;
}

int
getlon(char *s1, char *s2, Fix *f){
	double lon;
	static count;

	if(*s1 == 0 || ! isdigit(*s1) || strlen(s1) <= 5){
		badlon++;
		return -1;
	}
	lon = strtod(s1+3, nil);
	if(isNaN(lon)){
		badlon++;
		return -1;
	}
	lon /= 60.0;
	lon += 100*(s1[0] - '0') + 10*(s1[1] - '0') + s1[2] - '0';
	if(lon < 0 || lon > 180.0){
		badlon++;
		return -1;
	}
	switch(*s2){
	default:
		badlon++;
		return -1;
	case 'W':
		lon = -lon;
	case 'E':
		break;
	}
	if(f->lon <= 180.0 && count < 3 && fabs(f->lon - lon) > 10.0){
		count++;
		suspectlon++;
		return -1;
	}
	f->lon = lon;
	goodlon++;
	count = 0;
	return 0;
}

int
getmagvar(char *s1, char *s2, Fix *f){
	double magvar;

	if(*s1 == 0) return 0;
	if(isdigit(*s1) && strlen(s1) > 5){
		magvar = strtod(s1+3, nil);
		if(isNaN(magvar))
			return 0;
		magvar /= 60.0;
		magvar += 100*(s1[0] - '0') + 10*(s1[1] - '0') + s1[2] - '0';
		if(*s2 == 'W'){
			f->magvar = -magvar;
			return 1;
		}
		if(*s2 == 'E'){
			f->magvar = magvar;
			return 1;
		}
		return 0;
	}
	return 0;
}

void
putline(char *s){
	write(ttyfd, s, strlen(s));
	write(ttyfd, "\r\n", 2);
}

int
type(char *s){
	int i;

	for(i = 0; i < nelem(gpsmsg); i++){
		if(strcmp(s, gpsmsg[i].name) == 0) return i;
	}
	return -1;
}

void
setline(void){
	char *serialctl;

	serialctl = smprint("%sctl", serial);
	if((ttyfd = open(serial, ORDWR)) < 0)
		sysfatal("%s: %r", serial);
	if((ctlfd = open(serialctl, OWRITE)) >= 0){
		if(fprint(ctlfd, baudstr, baud) < 0)
			sysfatal("%s: %r", serialctl);
	}else
		gpsplayback = 0x8;
	free(serialctl);
}

int getonechar(vlong *t){
	static char buf[32], *p;
	static int n;

	if(n == 0){
		n = read(ttyfd, buf, sizeof(buf));
		if(t) *t = nsec();
		if(n < 0)
			sysfatal("%s: %r", serial);
		if(n == 0)
			threadexits(nil);
		/*
		 * We received n characters, so the first must have been there
		 * at least n/(10*baud) seconds (10 is 1 start
		 * bit, one stop bit and 8 data bits per character)
		 */
		if(t) {
			*t -= n * nsecperchar;
			histo[n]++;
		}
		p = buf;
	}
	n--;
	return *p++;
}

void
getline(char *s, int size, vlong *t){
	uchar c;
	char *p;
	int n, cs;

tryagain:
	for(;;){
		p = s;
		n = 0;
		while((c = getonechar(t)) != '\n' && n < size){
			t = nil;
			if(c != '\r'){
				*p++ = c;
				n++;
			}
		}
		if(n < size)
			break;
		while(getonechar(t) != '\n' && n < 4096)
			n++;
		if(n == 4096)
			sysfatal("preposterous gps line, wrong baud rate?");
		fprint(2, "ridiculous gps line: %d bytes\n", n);
	}
	*p = 0;
	for(p = s; isdigit(*p); p++)
		;
	if(*p++ == '	')
		memmove(s, p, strlen(p)+1);
	if(s[0] == '$'){
		if(n > 4 && s[n-3] == '*'){
			s[n-3] = 0;
			p = s+1;
			cs = 0;
			while(*p) cs ^= *p++;
			n = strtol(&s[n-2], nil, 16);
			if(n != cs){
				if(debug)
					fprint(2, "Checksum error %s, 0x%x, 0x%x\n",
						s, n, cs);
				checksumerrors++;
				goto tryagain;
			}
		}
	}
	for(p = s; *p; rawin++)
		raw[rawin & Rawmask] = *p++;
	raw[rawin & Rawmask] = '\n';
	rawin++;
}
