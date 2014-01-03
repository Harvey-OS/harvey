#include <u.h>
#include <libc.h>
#include <ctype.h>
#include <auth.h>
#include <fcall.h>
#include <draw.h>
#include <event.h>

#define	MAXNUM	10	/* maximum number of numbers on data line */

typedef struct Graph	Graph;
typedef struct Machine	Machine;

struct Graph
{
	int		colindex;
	Rectangle	r;
	uvlong		*data;
	int		ndata;
	char		*label;
	void		(*newvalue)(Machine*, uvlong*, uvlong*, int);
	void		(*update)(Graph*, uvlong, uvlong);
	Machine		*mach;
	int		overflow;
	Image		*overtmp;
};

enum
{
	/* old /dev/swap */
	Mem		= 0,
	Maxmem,
	Swap,
	Maxswap,

	/* /dev/sysstats */
	Procno	= 0,
	Context,
	Interrupt,
	Syscall,
	Fault,
	TLBfault,
	TLBpurge,
	Load,
	Idle,
	InIntr,
	/* /net/ether0/stats */
	In		= 0,
	Link,
	Out,
	Err0,
};

struct Machine
{
	char		*name;
	char		*shortname;
	int		remote;
	int		statsfd;
	int		swapfd;
	int		etherfd;
	int		ifstatsfd;
	int		batteryfd;
	int		bitsybatfd;
	int		tempfd;
	int		disable;

	uvlong		devswap[4];
	uvlong		devsysstat[10];
	uvlong		prevsysstat[10];
	int		nproc;
	int		lgproc;
	uvlong		netetherstats[8];
	uvlong		prevetherstats[8];
	uvlong		batterystats[2];
	uvlong		netetherifstats[2];
	uvlong		temp[10];

	/* big enough to hold /dev/sysstat even with many processors */
	char		buf[8*1024];
	char		*bufp;
	char		*ebufp;
};

enum
{
	Mainproc,
	Mouseproc,
	NPROC,
};

enum
{
	Ncolor		= 6,
	Ysqueeze	= 2,	/* vertical squeezing of label text */
	Labspace	= 2,	/* room around label */
	Dot		= 2,	/* height of dot */
	Opwid		= 5,	/* strlen("add  ") or strlen("drop ") */
	Nlab		= 3,	/* max number of labels on y axis */
	Lablen		= 16,	/* max length of label */
	Lx		= 4,	/* label tick length */
};

enum Menu2
{
	Mbattery,
	Mcontext,
	Mether,
	Methererr,
	Metherin,
	Metherout,
	Mfault,
	Midle,
	Minintr,
	Mintr,
	Mload,
	Mmem,
	Mswap,
	Msyscall,
	Mtlbmiss,
	Mtlbpurge,
	Msignal,
	Mtemp,
	Nmenu2,
};

char	*menu2str[Nmenu2+1] = {
	"add  battery ",
	"add  context ",
	"add  ether   ",
	"add  ethererr",
	"add  etherin ",
	"add  etherout",
	"add  fault   ",
	"add  idle    ",
	"add  inintr  ",
	"add  intr    ",
	"add  load    ",
	"add  mem     ",
	"add  swap    ",
	"add  syscall ",
	"add  tlbmiss ",
	"add  tlbpurge",
	"add  802.11b ",
	"add  temp    ",
	nil,
};


void	contextval(Machine*, uvlong*, uvlong*, int),
	etherval(Machine*, uvlong*, uvlong*, int),
	ethererrval(Machine*, uvlong*, uvlong*, int),
	etherinval(Machine*, uvlong*, uvlong*, int),
	etheroutval(Machine*, uvlong*, uvlong*, int),
	faultval(Machine*, uvlong*, uvlong*, int),
	intrval(Machine*, uvlong*, uvlong*, int),
	inintrval(Machine*, uvlong*, uvlong*, int),
	loadval(Machine*, uvlong*, uvlong*, int),
	idleval(Machine*, uvlong*, uvlong*, int),
	memval(Machine*, uvlong*, uvlong*, int),
	swapval(Machine*, uvlong*, uvlong*, int),
	syscallval(Machine*, uvlong*, uvlong*, int),
	tlbmissval(Machine*, uvlong*, uvlong*, int),
	tlbpurgeval(Machine*, uvlong*, uvlong*, int),
	batteryval(Machine*, uvlong*, uvlong*, int),
	signalval(Machine*, uvlong*, uvlong*, int),
	tempval(Machine*, uvlong*, uvlong*, int);

Menu	menu2 = {menu2str, nil};
int	present[Nmenu2];
void	(*newvaluefn[Nmenu2])(Machine*, uvlong*, uvlong*, int init) = {
	batteryval,
	contextval,
	etherval,
	ethererrval,
	etherinval,
	etheroutval,
	faultval,
	idleval,
	inintrval,
	intrval,
	loadval,
	memval,
	swapval,
	syscallval,
	tlbmissval,
	tlbpurgeval,
	signalval,
	tempval,
};

Image	*cols[Ncolor][3];
Graph	*graph;
Machine	*mach;
Font	*mediumfont;
char	*mysysname;
char	*mycputype;
char	argchars[] = "8bceEfiImlnpstwz";
int	pids[NPROC];
int 	parity;	/* toggled to avoid patterns in textured background */
int	nmach;
int	ngraph;	/* totaly number is ngraph*nmach */
double	scale = 1.0;
int	logscale = 0;
int	ylabels = 0;
int	oldsystem = 0;
int 	sleeptime = 1000;

char	*procnames[NPROC] = {"main", "mouse"};

void
killall(char *s)
{
	int i, pid;

	pid = getpid();
	for(i=0; i<NPROC; i++)
		if(pids[i] && pids[i]!=pid)
			postnote(PNPROC, pids[i], "kill");
	exits(s);
}

void*
emalloc(ulong sz)
{
	void *v;
	v = malloc(sz);
	if(v == nil) {
		fprint(2, "stats: out of memory allocating %ld: %r\n", sz);
		killall("mem");
	}
	memset(v, 0, sz);
	return v;
}

void*
erealloc(void *v, ulong sz)
{
	v = realloc(v, sz);
	if(v == nil) {
		fprint(2, "stats: out of memory reallocating %ld: %r\n", sz);
		killall("mem");
	}
	return v;
}

char*
estrdup(char *s)
{
	char *t;
	if((t = strdup(s)) == nil) {
		fprint(2, "stats: out of memory in strdup(%.10s): %r\n", s);
		killall("mem");
	}
	return t;
}

void
mkcol(int i, int c0, int c1, int c2)
{
	cols[i][0] = allocimagemix(display, c0, DWhite);
	cols[i][1] = allocimage(display, Rect(0,0,1,1), CMAP8, 1, c1);
	cols[i][2] = allocimage(display, Rect(0,0,1,1), CMAP8, 1, c2);
}

void
colinit(void)
{
	mediumfont = openfont(display, "/lib/font/bit/pelm/latin1.8.font");
	if(mediumfont == nil)
		mediumfont = font;

	/* Peach */
	mkcol(0, 0xFFAAAAFF, 0xFFAAAAFF, 0xBB5D5DFF);
	/* Aqua */
	mkcol(1, DPalebluegreen, DPalegreygreen, DPurpleblue);
	/* Yellow */
	mkcol(2, DPaleyellow, DDarkyellow, DYellowgreen);
	/* Green */
	mkcol(3, DPalegreen, DMedgreen, DDarkgreen);
	/* Blue */
	mkcol(4, 0x00AAFFFF, 0x00AAFFFF, 0x0088CCFF);
	/* Grey */
	cols[5][0] = allocimage(display, Rect(0,0,1,1), CMAP8, 1, 0xEEEEEEFF);
	cols[5][1] = allocimage(display, Rect(0,0,1,1), CMAP8, 1, 0xCCCCCCFF);
	cols[5][2] = allocimage(display, Rect(0,0,1,1), CMAP8, 1, 0x888888FF);
}

int
loadbuf(Machine *m, int *fd)
{
	int n;


	if(*fd < 0)
		return 0;
	seek(*fd, 0, 0);
	n = read(*fd, m->buf, sizeof m->buf-1);
	if(n <= 0){
		close(*fd);
		*fd = -1;
		return 0;
	}
	m->bufp = m->buf;
	m->ebufp = m->buf+n;
	m->buf[n] = 0;
	return 1;
}

void
label(Point p, int dy, char *text)
{
	char *s;
	Rune r[2];
	int w, maxw, maxy;

	p.x += Labspace;
	maxy = p.y+dy;
	maxw = 0;
	r[1] = '\0';
	for(s=text; *s; ){
		if(p.y+mediumfont->height-Ysqueeze > maxy)
			break;
		w = chartorune(r, s);
		s += w;
		w = runestringwidth(mediumfont, r);
		if(w > maxw)
			maxw = w;
		runestring(screen, p, display->black, ZP, mediumfont, r);
		p.y += mediumfont->height-Ysqueeze;
	}
}

Point
paritypt(int x)
{
	return Pt(x+parity, 0);
}

Point
datapoint(Graph *g, int x, uvlong v, uvlong vmax)
{
	Point p;
	double y;

	p.x = x;
	y = ((double)v)/(vmax*scale);
	if(logscale){
		/*
		 * Arrange scale to cover a factor of 1000.
		 * vmax corresponds to the 100 mark.
		 * 10*vmax is the top of the scale.
		 */
		if(y <= 0.)
			y = 0;
		else{
			y = log10(y);
			/* 1 now corresponds to the top; -2 to the bottom; rescale */
			y = (y+2.)/3.;
		}
	}
	if(y < 0x7fffffff){	/* avoid floating overflow */
		p.y = g->r.max.y - Dy(g->r)*y - Dot;
		if(p.y < g->r.min.y)
			p.y = g->r.min.y;
		if(p.y > g->r.max.y-Dot)
			p.y = g->r.max.y-Dot;
	}else
		p.y = g->r.max.y-Dot;
	return p;
}

void
drawdatum(Graph *g, int x, uvlong prev, uvlong v, uvlong vmax)
{
	int c;
	Point p, q;

	c = g->colindex;
	p = datapoint(g, x, v, vmax);
	q = datapoint(g, x, prev, vmax);
	if(p.y < q.y){
		draw(screen, Rect(p.x, g->r.min.y, p.x+1, p.y), cols[c][0], nil, paritypt(p.x));
		draw(screen, Rect(p.x, p.y, p.x+1, q.y+Dot), cols[c][2], nil, ZP);
		draw(screen, Rect(p.x, q.y+Dot, p.x+1, g->r.max.y), cols[c][1], nil, ZP);
	}else{
		draw(screen, Rect(p.x, g->r.min.y, p.x+1, q.y), cols[c][0], nil, paritypt(p.x));
		draw(screen, Rect(p.x, q.y, p.x+1, p.y+Dot), cols[c][2], nil, ZP);
		draw(screen, Rect(p.x, p.y+Dot, p.x+1, g->r.max.y), cols[c][1], nil, ZP);
	}

}

void
redraw(Graph *g, uvlong vmax)
{
	int i, c;

	c = g->colindex;
	draw(screen, g->r, cols[c][0], nil, paritypt(g->r.min.x));
	for(i=1; i<Dx(g->r); i++)
		drawdatum(g, g->r.max.x-i, g->data[i-1], g->data[i], vmax);
	drawdatum(g, g->r.min.x, g->data[i], g->data[i], vmax);
	g->overflow = 0;
}

void
update1(Graph *g, uvlong v, uvlong vmax)
{
	char buf[48];
	int overflow;

	if(g->overflow && g->overtmp!=nil)
		draw(screen, g->overtmp->r, g->overtmp, nil, g->overtmp->r.min);
	draw(screen, g->r, screen, nil, Pt(g->r.min.x+1, g->r.min.y));
	drawdatum(g, g->r.max.x-1, g->data[0], v, vmax);
	memmove(g->data+1, g->data, (g->ndata-1)*sizeof(g->data[0]));
	g->data[0] = v;
	g->overflow = 0;
	if(logscale)
		overflow = (v>10*vmax*scale);
	else
		overflow = (v>vmax*scale);
	if(overflow && g->overtmp!=nil){
		g->overflow = 1;
		draw(g->overtmp, g->overtmp->r, screen, nil, g->overtmp->r.min);
		sprint(buf, "%llud", v);
		string(screen, g->overtmp->r.min, display->black, ZP, mediumfont, buf);
	}
}

/* read one line of text from buffer and process integers */
int
readnums(Machine *m, int n, uvlong *a, int spanlines)
{
	int i;
	char *p, *q, *ep;

	if(spanlines)
		ep = m->ebufp;
	else
		for(ep=m->bufp; ep<m->ebufp; ep++)
			if(*ep == '\n')
				break;
	p = m->bufp;
	for(i=0; i<n && p<ep; i++){
		while(p<ep && (!isascii(*p) || !isdigit(*p)) && *p!='-')
			p++;
		if(p == ep)
			break;
		a[i] = strtoull(p, &q, 10);
		p = q;
	}
	if(ep < m->ebufp)
		ep++;
	m->bufp = ep;
	return i == n;
}

/* Network on fd1, mount driver on fd0 */
static int
filter(int fd)
{
	int p[2];

	if(pipe(p) < 0){
		fprint(2, "stats: can't pipe: %r\n");
		killall("pipe");
	}

	switch(rfork(RFNOWAIT|RFPROC|RFFDG)) {
	case -1:
		sysfatal("rfork record module");
	case 0:
		dup(fd, 1);
		close(fd);
		dup(p[0], 0);
		close(p[0]);
		close(p[1]);
		execl("/bin/aux/fcall", "fcall", nil);
		fprint(2, "stats: can't exec fcall: %r\n");
		killall("fcall");
	default:
		close(fd);
		close(p[0]);
	}
	return p[1];
}

/*
 * 9fs
 */
int
connect9fs(char *addr)
{
	char dir[256], *na;
	int fd;

	fprint(2, "connect9fs...");
	na = netmkaddr(addr, 0, "9fs");

	fprint(2, "dial %s...", na);
	if((fd = dial(na, 0, dir, 0)) < 0)
		return -1;

	fprint(2, "dir %s...", dir);
//	if(strstr(dir, "tcp"))
//		fd = filter(fd);
	return fd;
}

int
old9p(int fd)
{
	int p[2];

	if(pipe(p) < 0)
		return -1;

	switch(rfork(RFPROC|RFFDG|RFNAMEG)) {
	case -1:
		return -1;
	case 0:
		if(fd != 1){
			dup(fd, 1);
			close(fd);
		}
		if(p[0] != 0){
			dup(p[0], 0);
			close(p[0]);
		}
		close(p[1]);
		if(0){
			fd = open("/sys/log/cpu", OWRITE);
			if(fd != 2){
				dup(fd, 2);
				close(fd);
			}
			execl("/bin/srvold9p", "srvold9p", "-ds", nil);
		} else
			execl("/bin/srvold9p", "srvold9p", "-s", nil);
		return -1;
	default:
		close(fd);
		close(p[0]);
	}
	return p[1];
}


/*
 * exportfs
 */
int
connectexportfs(char *addr)
{
	char buf[ERRMAX], dir[256], *na;
	int fd, n;
	char *tree;
	AuthInfo *ai;

	tree = "/";
	na = netmkaddr(addr, 0, "exportfs");
	if((fd = dial(na, 0, dir, 0)) < 0)
		return -1;

	ai = auth_proxy(fd, auth_getkey, "proto=p9any role=client");
	if(ai == nil)
		return -1;

	n = write(fd, tree, strlen(tree));
	if(n < 0){
		close(fd);
		return -1;
	}

	strcpy(buf, "can't read tree");
	n = read(fd, buf, sizeof buf - 1);
	if(n!=2 || buf[0]!='O' || buf[1]!='K'){
		buf[sizeof buf - 1] = '\0';
		werrstr("bad remote tree: %s\n", buf);
		close(fd);
		return -1;
	}

//	if(strstr(dir, "tcp"))
//		fd = filter(fd);

	if(oldsystem)
		return old9p(fd);

	return fd;
}

int
readswap(Machine *m, uvlong *a)
{
	if(strstr(m->buf, "memory\n")){
		/* new /dev/swap - skip first 3 numbers */
		if(!readnums(m, 7, a, 1))
			return 0;
		a[0] = a[3];
		a[1] = a[4];
		a[2] = a[5];
		a[3] = a[6];
		return 1;
	}
	return readnums(m, nelem(m->devswap), a, 0);
}

char*
shortname(char *s)
{
	char *p, *e;

	p = estrdup(s);
	e = strchr(p, '.');
	if(e)
		*e = 0;
	return p;
}

int
ilog10(uvlong j)
{
	int i;

	for(i = 0; j >= 10; i++)
		j /= 10;
	return i;
}

int
initmach(Machine *m, char *name)
{
	int n, fd;
	uvlong a[MAXNUM];
	char *p, mpt[256], buf[256];

	p = strchr(name, '!');
	if(p)
		p++;
	else
		p = name;
	m->name = estrdup(p);
	m->shortname = shortname(p);
	m->remote = (strcmp(p, mysysname) != 0);
	if(m->remote == 0)
		strcpy(mpt, "");
	else{
		snprint(mpt, sizeof mpt, "/n/%s", p);
		fd = connectexportfs(name);
		if(fd < 0){
			fprint(2, "can't connect to %s: %r\n", name);
			return 0;
		}
		/* BUG? need to use amount() now? */
		if(mount(fd, -1, mpt, MREPL, "") < 0){
			fprint(2, "stats: mount %s on %s failed (%r); trying /n/sid\n", name, mpt);
			strcpy(mpt, "/n/sid");
			if(mount(fd, -1, mpt, MREPL, "") < 0){
				fprint(2, "stats: mount %s on %s failed: %r\n", name, mpt);
				return 0;
			}
		}
	}

	snprint(buf, sizeof buf, "%s/dev/swap", mpt);
	m->swapfd = open(buf, OREAD);
	if(loadbuf(m, &m->swapfd) && readswap(m, a))
		memmove(m->devswap, a, sizeof m->devswap);
	else{
		m->devswap[Maxswap] = 100;
		m->devswap[Maxmem] = 100;
	}

	snprint(buf, sizeof buf, "%s/dev/sysstat", mpt);
	m->statsfd = open(buf, OREAD);
	if(loadbuf(m, &m->statsfd)){
		for(n=0; readnums(m, nelem(m->devsysstat), a, 0); n++)
			;
		m->nproc = n;
	}else
		m->nproc = 1;
	m->lgproc = ilog10(m->nproc);

	snprint(buf, sizeof buf, "%s/net/ether0/stats", mpt);
	m->etherfd = open(buf, OREAD);
	if(loadbuf(m, &m->etherfd) && readnums(m, nelem(m->netetherstats), a, 1))
		memmove(m->netetherstats, a, sizeof m->netetherstats);

	snprint(buf, sizeof buf, "%s/net/ether0/ifstats", mpt);
	m->ifstatsfd = open(buf, OREAD);
	if(loadbuf(m, &m->ifstatsfd)){
		/* need to check that this is a wavelan interface */
		if(strncmp(m->buf, "Signal: ", 8) == 0 && readnums(m, nelem(m->netetherifstats), a, 1))
			memmove(m->netetherifstats, a, sizeof m->netetherifstats);
	}

	snprint(buf, sizeof buf, "%s/mnt/apm/battery", mpt);
	m->batteryfd = open(buf, OREAD);
	m->bitsybatfd = -1;
	if(m->batteryfd >= 0){
		if(loadbuf(m, &m->batteryfd) && readnums(m, nelem(m->batterystats), a, 0))
			memmove(m->batterystats, a, sizeof(m->batterystats));
	}else{
		snprint(buf, sizeof buf, "%s/dev/battery", mpt);
		m->bitsybatfd = open(buf, OREAD);
		if(loadbuf(m, &m->bitsybatfd) && readnums(m, 1, a, 0))
			memmove(m->batterystats, a, sizeof(m->batterystats));
	}
	snprint(buf, sizeof buf, "%s/dev/cputemp", mpt);
	m->tempfd = open(buf, OREAD);
	if(loadbuf(m, &m->tempfd))
		for(n=0; n < nelem(m->temp) && readnums(m, 2, a, 0); n++)
			 m->temp[n] = a[0];
	return 1;
}

jmp_buf catchalarm;

void
alarmed(void *a, char *s)
{
	if(strcmp(s, "alarm") == 0)
		notejmp(a, catchalarm, 1);
	noted(NDFLT);
}

int
needswap(int init)
{
	return init | present[Mmem] | present[Mswap];
}


int
needstat(int init)
{
	return init | present[Mcontext]  | present[Mfault] | present[Mintr] | present[Mload] | present[Midle] |
		present[Minintr] | present[Msyscall] | present[Mtlbmiss] | present[Mtlbpurge];
}


int
needether(int init)
{
	return init | present[Mether] | present[Metherin] | present[Metherout] | present[Methererr];
}

int
needbattery(int init)
{
	return init | present[Mbattery];
}

int
needsignal(int init)
{
	return init | present[Msignal];
}

int
needtemp(int init)
{
	return init | present[Mtemp];
}

void
readmach(Machine *m, int init)
{
	int n, i;
	uvlong a[nelem(m->devsysstat)];
	char buf[32];

	if(m->remote && (m->disable || setjmp(catchalarm))){
		if (m->disable++ >= 5)
			m->disable = 0; /* give it another chance */
		memmove(m->devsysstat, m->prevsysstat, sizeof m->devsysstat);
		memmove(m->netetherstats, m->prevetherstats, sizeof m->netetherstats);
		return;
	}
	snprint(buf, sizeof buf, "%s", m->name);
	if (strcmp(m->name, buf) != 0){
		free(m->name);
		m->name = estrdup(buf);
		free(m->shortname);
		m->shortname = shortname(buf);
		if(display != nil)	/* else we're still initializing */
			eresized(0);
	}
	if(m->remote){
		notify(alarmed);
		alarm(5000);
	}
	if(needswap(init) && loadbuf(m, &m->swapfd) && readswap(m, a))
		memmove(m->devswap, a, sizeof m->devswap);
	if(needstat(init) && loadbuf(m, &m->statsfd)){
		memmove(m->prevsysstat, m->devsysstat, sizeof m->devsysstat);
		memset(m->devsysstat, 0, sizeof m->devsysstat);
		for(n=0; n<m->nproc && readnums(m, nelem(m->devsysstat), a, 0); n++)
			for(i=0; i<nelem(m->devsysstat); i++)
				m->devsysstat[i] += a[i];
	}
	if(needether(init) && loadbuf(m, &m->etherfd) && readnums(m, nelem(m->netetherstats), a, 1)){
		memmove(m->prevetherstats, m->netetherstats, sizeof m->netetherstats);
		memmove(m->netetherstats, a, sizeof m->netetherstats);
	}
	if(needsignal(init) && loadbuf(m, &m->ifstatsfd) && strncmp(m->buf, "Signal: ", 8)==0 && readnums(m, nelem(m->netetherifstats), a, 1)){
		memmove(m->netetherifstats, a, sizeof m->netetherifstats);
	}
	if(needbattery(init) && loadbuf(m, &m->batteryfd) && readnums(m, nelem(m->batterystats), a, 0))
		memmove(m->batterystats, a, sizeof(m->batterystats));
	if(needbattery(init) && loadbuf(m, &m->bitsybatfd) && readnums(m, 1, a, 0))
		memmove(m->batterystats, a, sizeof(m->batterystats));
	if(needtemp(init) && loadbuf(m, &m->tempfd))
		for(n=0; n < nelem(m->temp) && readnums(m, 2, a, 0); n++)
			 m->temp[n] = a[0];
	if(m->remote){
		alarm(0);
		notify(nil);
	}
}

void
memval(Machine *m, uvlong *v, uvlong *vmax, int)
{
	*v = m->devswap[Mem];
	*vmax = m->devswap[Maxmem];
}

void
swapval(Machine *m, uvlong *v, uvlong *vmax, int)
{
	*v = m->devswap[Swap];
	*vmax = m->devswap[Maxswap];
}

void
contextval(Machine *m, uvlong *v, uvlong *vmax, int init)
{
	*v = m->devsysstat[Context]-m->prevsysstat[Context];
	*vmax = sleeptime*m->nproc;
	if(init)
		*vmax = sleeptime;
}

/*
 * bug: need to factor in HZ
 */
void
intrval(Machine *m, uvlong *v, uvlong *vmax, int init)
{
	*v = m->devsysstat[Interrupt]-m->prevsysstat[Interrupt];
	*vmax = sleeptime*m->nproc*10;
	if(init)
		*vmax = sleeptime*10;
}

void
syscallval(Machine *m, uvlong *v, uvlong *vmax, int init)
{
	*v = m->devsysstat[Syscall]-m->prevsysstat[Syscall];
	*vmax = sleeptime*m->nproc;
	if(init)
		*vmax = sleeptime;
}

void
faultval(Machine *m, uvlong *v, uvlong *vmax, int init)
{
	*v = m->devsysstat[Fault]-m->prevsysstat[Fault];
	*vmax = sleeptime*m->nproc;
	if(init)
		*vmax = sleeptime;
}

void
tlbmissval(Machine *m, uvlong *v, uvlong *vmax, int init)
{
	*v = m->devsysstat[TLBfault]-m->prevsysstat[TLBfault];
	*vmax = (sleeptime/1000)*10*m->nproc;
	if(init)
		*vmax = (sleeptime/1000)*10;
	if (mycputype && strcmp(mycputype, "mips") == 0)
		*vmax *= 50000;		/* mainly for 16-entry tlbs (rb) */	
}

void
tlbpurgeval(Machine *m, uvlong *v, uvlong *vmax, int init)
{
	*v = m->devsysstat[TLBpurge]-m->prevsysstat[TLBpurge];
	*vmax = (sleeptime/1000)*10*m->nproc;
	if(init)
		*vmax = (sleeptime/1000)*10;
}

void
loadval(Machine *m, uvlong *v, uvlong *vmax, int init)
{
	*v = m->devsysstat[Load];
	*vmax = 1000*m->nproc;
	if(init)
		*vmax = 1000;
}

void
idleval(Machine *m, uvlong *v, uvlong *vmax, int)
{
	*v = m->devsysstat[Idle]/m->nproc;
	*vmax = 100;
}

void
inintrval(Machine *m, uvlong *v, uvlong *vmax, int)
{
	*v = m->devsysstat[InIntr]/m->nproc;
	*vmax = 100;
}

void
etherval(Machine *m, uvlong *v, uvlong *vmax, int init)
{
	*v = m->netetherstats[In]-m->prevetherstats[In] + m->netetherstats[Out]-m->prevetherstats[Out];
	*vmax = sleeptime*m->nproc;
	if(init)
		*vmax = sleeptime;
}

void
etherinval(Machine *m, uvlong *v, uvlong *vmax, int init)
{
	*v = m->netetherstats[In]-m->prevetherstats[In];
	*vmax = sleeptime*m->nproc;
	if(init)
		*vmax = sleeptime;
}

void
etheroutval(Machine *m, uvlong *v, uvlong *vmax, int init)
{
	*v = m->netetherstats[Out]-m->prevetherstats[Out];
	*vmax = sleeptime*m->nproc;
	if(init)
		*vmax = sleeptime;
}

void
ethererrval(Machine *m, uvlong *v, uvlong *vmax, int init)
{
	int i;

	*v = 0;
	for(i=Err0; i<nelem(m->netetherstats); i++)
		*v += m->netetherstats[i];
	*vmax = (sleeptime/1000)*10*m->nproc;
	if(init)
		*vmax = (sleeptime/1000)*10;
}

void
batteryval(Machine *m, uvlong *v, uvlong *vmax, int)
{
	*v = m->batterystats[0];
	if(m->bitsybatfd >= 0)
		*vmax = 184;		// at least on my bitsy...
	else
		*vmax = 100;
}

void
signalval(Machine *m, uvlong *v, uvlong *vmax, int)
{
	ulong l;

	*vmax = sleeptime;
	l = m->netetherifstats[0];
	/*
	 * Range is seen to be from about -45 (strong) to -95 (weak); rescale
	 */
	if(l == 0){	/* probably not present */
		*v = 0;
		return;
	}
	*v = 20*(l+95);
}

void
tempval(Machine *m, uvlong *v, uvlong *vmax, int)
{
	ulong l;

	*vmax = sleeptime;
	l = m->temp[0];
	if(l == ~0 || l == 0)
		*v = 0;
	else
		*v = (l-20)*27;
}

void
usage(void)
{
	fprint(2, "usage: stats [-O] [-S scale] [-LY] [-%s] [machine...]\n", argchars);
	exits("usage");
}

void
addgraph(int n)
{
	Graph *g, *ograph;
	int i, j;
	static int nadd;

	if(n > nelem(menu2str))
		abort();
	/* avoid two adjacent graphs of same color */
	if(ngraph>0 && graph[ngraph-1].colindex==nadd%Ncolor)
		nadd++;
	ograph = graph;
	graph = emalloc(nmach*(ngraph+1)*sizeof(Graph));
	for(i=0; i<nmach; i++)
		for(j=0; j<ngraph; j++)
			graph[i*(ngraph+1)+j] = ograph[i*ngraph+j];
	free(ograph);
	ngraph++;
	for(i=0; i<nmach; i++){
		g = &graph[i*ngraph+(ngraph-1)];
		memset(g, 0, sizeof(Graph));
		g->label = menu2str[n]+Opwid;
		g->newvalue = newvaluefn[n];
		g->update = update1;	/* no other update functions yet */
		g->mach = &mach[i];
		g->colindex = nadd%Ncolor;
	}
	present[n] = 1;
	nadd++;
}

void
dropgraph(int which)
{
	Graph *ograph;
	int i, j, n;

	if(which > nelem(menu2str))
		abort();
	/* convert n to index in graph table */
	n = -1;
	for(i=0; i<ngraph; i++)
		if(strcmp(menu2str[which]+Opwid, graph[i].label) == 0){
			n = i;
			break;
		}
	if(n < 0){
		fprint(2, "stats: internal error can't drop graph\n");
		killall("error");
	}
	ograph = graph;
	graph = emalloc(nmach*(ngraph-1)*sizeof(Graph));
	for(i=0; i<nmach; i++){
		for(j=0; j<n; j++)
			graph[i*(ngraph-1)+j] = ograph[i*ngraph+j];
		free(ograph[i*ngraph+j].data);
		freeimage(ograph[i*ngraph+j].overtmp);
		for(j++; j<ngraph; j++)
			graph[i*(ngraph-1)+j-1] = ograph[i*ngraph+j];
	}
	free(ograph);
	ngraph--;
	present[which] = 0;
}

int
addmachine(char *name)
{
	if(ngraph > 0){
		fprint(2, "stats: internal error: ngraph>0 in addmachine()\n");
		usage();
	}
	if(mach == nil)
		nmach = 0;	/* a little dance to get us started with local machine by default */
	mach = erealloc(mach, (nmach+1)*sizeof(Machine));
	memset(mach+nmach, 0, sizeof(Machine));
	if (initmach(mach+nmach, name)){
		nmach++;
		return 1;
	} else
		return 0;
}

void
labelstrs(Graph *g, char strs[Nlab][Lablen], int *np)
{
	int j;
	uvlong v, vmax;

	g->newvalue(g->mach, &v, &vmax, 1);
	if(logscale){
		for(j=1; j<=2; j++)
			sprint(strs[j-1], "%g", scale*pow(10., j)*(double)vmax/100.);
		*np = 2;
	}else{
		for(j=1; j<=3; j++)
			sprint(strs[j-1], "%g", scale*(double)j*(double)vmax/4.0);
		*np = 3;
	}
}

int
labelwidth(void)
{
	int i, j, n, w, maxw;
	char strs[Nlab][Lablen];

	maxw = 0;
	for(i=0; i<ngraph; i++){
		/* choose value for rightmost graph */
		labelstrs(&graph[ngraph*(nmach-1)+i], strs, &n);
		for(j=0; j<n; j++){
			w = stringwidth(mediumfont, strs[j]);
			if(w > maxw)
				maxw = w;
		}
	}
	return maxw;
}

void
resize(void)
{
	int i, j, k, n, startx, starty, x, y, dx, dy, ly, ondata, maxx, wid, nlab;
	Graph *g;
	Rectangle machr, r;
	uvlong v, vmax;
	char buf[128], labs[Nlab][Lablen];

	draw(screen, screen->r, display->white, nil, ZP);

	/* label left edge */
	x = screen->r.min.x;
	y = screen->r.min.y + Labspace+mediumfont->height+Labspace;
	dy = (screen->r.max.y - y)/ngraph;
	dx = Labspace+stringwidth(mediumfont, "0")+Labspace;
	startx = x+dx+1;
	starty = y;
	for(i=0; i<ngraph; i++,y+=dy){
		draw(screen, Rect(x, y-1, screen->r.max.x, y), display->black, nil, ZP);
		draw(screen, Rect(x, y, x+dx, screen->r.max.y), cols[graph[i].colindex][0], nil, paritypt(x));
		label(Pt(x, y), dy, graph[i].label);
		draw(screen, Rect(x+dx, y, x+dx+1, screen->r.max.y), cols[graph[i].colindex][2], nil, ZP);
	}

	/* label top edge */
	dx = (screen->r.max.x - startx)/nmach;
	for(x=startx, i=0; i<nmach; i++,x+=dx){
		draw(screen, Rect(x-1, starty-1, x, screen->r.max.y), display->black, nil, ZP);
		j = dx/stringwidth(mediumfont, "0");
		n = mach[i].nproc;
		if(n>1 && j>=1+3+mach[i].lgproc){	/* first char of name + (n) */
			j -= 3+mach[i].lgproc;
			if(j <= 0)
				j = 1;
			snprint(buf, sizeof buf, "%.*s(%d)", j, mach[i].shortname, n);
		}else
			snprint(buf, sizeof buf, "%.*s", j, mach[i].shortname);
		string(screen, Pt(x+Labspace, screen->r.min.y + Labspace), display->black, ZP, mediumfont, buf);
	}

	maxx = screen->r.max.x;

	/* label right, if requested */
	if(ylabels && dy>Nlab*(mediumfont->height+1)){
		wid = labelwidth();
		if(wid < (maxx-startx)-30){
			/* else there's not enough room */
			maxx -= 1+Lx+wid;
			draw(screen, Rect(maxx, starty, maxx+1, screen->r.max.y), display->black, nil, ZP);
			y = starty;
			for(j=0; j<ngraph; j++, y+=dy){
				/* choose value for rightmost graph */
				g = &graph[ngraph*(nmach-1)+j];
				labelstrs(g, labs, &nlab);
				r = Rect(maxx+1, y, screen->r.max.x, y+dy-1);
				if(j == ngraph-1)
					r.max.y = screen->r.max.y;
				draw(screen, r, cols[g->colindex][0], nil, paritypt(r.min.x));
				for(k=0; k<nlab; k++){
					ly = y + (dy*(nlab-k)/(nlab+1));
					draw(screen, Rect(maxx+1, ly, maxx+1+Lx, ly+1), display->black, nil, ZP);
					ly -= mediumfont->height/2;
					string(screen, Pt(maxx+1+Lx, ly), display->black, ZP, mediumfont, labs[k]);
				}
			}
		}
	}

	/* create graphs */
	for(i=0; i<nmach; i++){
		machr = Rect(startx+i*dx, starty, maxx, screen->r.max.y);
		if(i < nmach-1)
			machr.max.x = startx+(i+1)*dx - 1;
		y = starty;
		for(j=0; j<ngraph; j++, y+=dy){
			g = &graph[i*ngraph+j];
			/* allocate data */
			ondata = g->ndata;
			g->ndata = Dx(machr)+1;	/* may be too many if label will be drawn here; so what? */
			g->data = erealloc(g->data, g->ndata*sizeof(g->data[0]));
			if(g->ndata > ondata)
				memset(g->data+ondata, 0, (g->ndata-ondata)*sizeof(g->data[0]));
			/* set geometry */
			g->r = machr;
			g->r.min.y = y;
			g->r.max.y = y+dy - 1;
			if(j == ngraph-1)
				g->r.max.y = screen->r.max.y;
			draw(screen, g->r, cols[g->colindex][0], nil, paritypt(g->r.min.x));
			g->overflow = 0;
			r = g->r;
			r.max.y = r.min.y+mediumfont->height;
			r.max.x = r.min.x+stringwidth(mediumfont, "999999999999");
			freeimage(g->overtmp);
			g->overtmp = nil;
			if(r.max.x <= g->r.max.x)
				g->overtmp = allocimage(display, r, screen->chan, 0, -1);
			g->newvalue(g->mach, &v, &vmax, 0);
			redraw(g, vmax);
		}
	}

	flushimage(display, 1);
}

void
eresized(int new)
{
	lockdisplay(display);
	if(new && getwindow(display, Refnone) < 0) {
		fprint(2, "stats: can't reattach to window\n");
		killall("reattach");
	}
	resize();
	unlockdisplay(display);
}

void
mouseproc(void)
{
	Mouse mouse;
	int i;

	for(;;){
		mouse = emouse();
		if(mouse.buttons == 4){
			lockdisplay(display);
			for(i=0; i<Nmenu2; i++)
				if(present[i])
					memmove(menu2str[i], "drop ", Opwid);
				else
					memmove(menu2str[i], "add  ", Opwid);
			i = emenuhit(3, &mouse, &menu2);
			if(i >= 0){
				if(!present[i])
					addgraph(i);
				else if(ngraph > 1)
					dropgraph(i);
				resize();
			}
			unlockdisplay(display);
		}
	}
}

void
startproc(void (*f)(void), int index)
{
	int pid;

	switch(pid = rfork(RFPROC|RFMEM|RFNOWAIT)){
	case -1:
		fprint(2, "stats: fork failed: %r\n");
		killall("fork failed");
	case 0:
		f();
		fprint(2, "stats: %s process exits\n", procnames[index]);
		if(index >= 0)
			killall("process died");
		exits(nil);
	}
	if(index >= 0)
		pids[index] = pid;
}

void
main(int argc, char *argv[])
{
	int i, j;
	double secs;
	uvlong v, vmax, nargs;
	char args[100];

	nmach = 1;
	mysysname = getenv("sysname");
	if(mysysname == nil){
		fprint(2, "stats: can't find $sysname: %r\n");
		exits("sysname");
	}
	mysysname = estrdup(mysysname);
	mycputype = getenv("cputype");

	nargs = 0;
	ARGBEGIN{
	case 'T':
		secs = atof(EARGF(usage()));
		if(secs > 0)
			sleeptime = 1000*secs;
		break;
	case 'S':
		scale = atof(EARGF(usage()));
		if(scale <= 0)
			usage();
		break;
	case 'L':
		logscale++;
		break;
	case 'Y':
		ylabels++;
		break;
	case 'O':
		oldsystem = 1;
		break;
	default:
		if(nargs>=sizeof args || strchr(argchars, ARGC())==nil)
			usage();
		args[nargs++] = ARGC();
	}ARGEND

	if(argc == 0){
		mach = emalloc(nmach*sizeof(Machine));
		initmach(&mach[0], mysysname);
		readmach(&mach[0], 1);
	}else{
		for(i=j=0; i<argc; i++){
			if (addmachine(argv[i]))
				readmach(&mach[j++], 1);
		}
		if (j == 0)
			exits("connect");
	}

	for(i=0; i<nargs; i++)
	switch(args[i]){
	default:
		fprint(2, "stats: internal error: unknown arg %c\n", args[i]);
		usage();
	case 'b':
		addgraph(Mbattery);
		break;
	case 'c':
		addgraph(Mcontext);
		break;
	case 'e':
		addgraph(Mether);
		break;
	case 'E':
		addgraph(Metherin);
		addgraph(Metherout);
		break;
	case 'f':
		addgraph(Mfault);
		break;
	case 'i':
		addgraph(Mintr);
		break;
	case 'I':
		addgraph(Mload);
		addgraph(Midle);
		addgraph(Minintr);
		break;
	case 'l':
		addgraph(Mload);
		break;
	case 'm':
		addgraph(Mmem);
		break;
	case 'n':
		addgraph(Metherin);
		addgraph(Metherout);
		addgraph(Methererr);
		break;
	case 'p':
		addgraph(Mtlbpurge);
		break;
	case 's':
		addgraph(Msyscall);
		break;
	case 't':
		addgraph(Mtlbmiss);
		addgraph(Mtlbpurge);
		break;
	case '8':
		addgraph(Msignal);
		break;
	case 'w':
		addgraph(Mswap);
		break;
	case 'z':
		addgraph(Mtemp);
		break;
	}

	if(ngraph == 0)
		addgraph(Mload);

	for(i=0; i<nmach; i++)
		for(j=0; j<ngraph; j++)
			graph[i*ngraph+j].mach = &mach[i];

	if(initdraw(nil, nil, "stats") < 0){
		fprint(2, "stats: initdraw failed: %r\n");
		exits("initdraw");
	}
	colinit();
	einit(Emouse);
	notify(nil);
	startproc(mouseproc, Mouseproc);
	pids[Mainproc] = getpid();
	display->locking = 1;	/* tell library we're using the display lock */

	resize();

	unlockdisplay(display); /* display is still locked from initdraw() */
	for(;;){
		for(i=0; i<nmach; i++)
			readmach(&mach[i], 0);
		lockdisplay(display);
		parity = 1-parity;
		for(i=0; i<nmach*ngraph; i++){
			graph[i].newvalue(graph[i].mach, &v, &vmax, 0);
			graph[i].update(&graph[i], v, vmax);
		}
		flushimage(display, 1);
		unlockdisplay(display);
		sleep(sleeptime);
	}
}
