#include <u.h>
#include <libc.h>
#include <ctype.h>
#include <auth.h>
#include <fcall.h>
#include <draw.h>
#include <event.h>
#include <ip.h>
#include "icmp.h"

#define	MAXNUM	8	/* maximum number of numbers on data line */

typedef struct Graph	Graph;
typedef struct Machine	Machine;
typedef struct Req	Req;

enum {
	Gmsglen	= 16,
};

struct Graph
{
	int		colindex;
	Rectangle	r;
	long		*data;
	int		ndata;
	char		*label;
	void		(*newvalue)(Machine*, long*, long*, long*);
	void		(*update)(Graph*, long, long, long);
	Machine		*mach;
	int		overflow;
	Image		*overtmp;
	int		overtmplen;
	char		msg[Gmsglen];
	int		cursor;
	int		vmax;
};

enum
{
	MSGLEN		= 64,

	Rttmax		= 50,
};

struct Req
{
	int	seq;	/* sequence number */
	vlong	time;	/* time sent */
//	int	rtt;
	Req	*next;
};

struct Machine
{
	Lock;
	char	*name;
	int	pingfd;
	int	nproc;

	int	rttmsgs;
	ulong	rttsum;
	ulong	lastrtt;

	int	lostmsgs;
	int	rcvdmsgs;
	ulong	lostavg;
	int	unreachable;

	ushort	seq;
	Req	*first;
	Req	*last;
	Req	*rcvd;

	char	buf[1024];
	char	*bufp;
	char	*ebufp;
};

enum
{
	Ncolor		= 6,
	Ysqueeze	= 2,	/* vertical squeezing of label text */
	Labspace	= 2,	/* room around label */
	Dot		= 2,	/* height of dot */
	Opwid		= 5,	/* strlen("add  ") or strlen("drop ") */
	NPROC		= 128,
	NMACH		= 32,
};

enum Menu2
{
	Mrtt,
	Mlost,
	Nmenu2,
};

char	*menu2str[Nmenu2+1] = {
	"add  sec rtt",
	"add  % lost ",
	nil,
};


void	rttval(Machine*, long*, long*, long*);
void	lostval(Machine*, long*, long*, long*);

Menu	menu2 = {menu2str, nil};
int		present[Nmenu2];
void		(*newvaluefn[Nmenu2])(Machine*, long*, long*, long*) = {
	rttval,
	lostval,
};

Image		*cols[Ncolor][3];
Graph		*graph;
Machine		mach[NMACH];
Font		*mediumfont;
int		pids[NPROC];
int		npid;
int 		parity;	/* toggled to avoid patterns in textured background */
int		nmach;
int		ngraph;	/* totaly number is ngraph*nmach */
long		starttime;
int		pinginterval;

void	dropgraph(int);
void	addgraph(int);
void	startproc(void (*)(void*), void*);
void	resize(void);
long	rttscale(long);
int	which2index(int);
int	index2which(int);

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
		fprint(2, "%s: out of memory allocating %ld: %r\n", argv0, sz);
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
		fprint(2, "%s: out of memory reallocating %ld: %r\n", argv0, sz);
		killall("mem");
	}
	return v;
}

char*
estrdup(char *s)
{
	char *t;
	if((t = strdup(s)) == nil) {
		fprint(2, "%s: out of memory in strdup(%.10s): %r\n", argv0, s);
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
	n = read(*fd, m->buf, sizeof m->buf);
	if(n <= 0){
		close(*fd);
		*fd = -1;
		return 0;
	}
	m->bufp = m->buf;
	m->ebufp = m->buf+n;
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

void
hashmark(Point p, int dy, long v, long vmax, char *label)
{
	int y;
	int x;

	x = p.x + Labspace;
	y = p.y + (dy*(vmax-v))/vmax;
	draw(screen, Rect(p.x, y-1, p.x+Labspace, y+1), display->black, nil, ZP);
	if(dy > 5*mediumfont->height)
		string(screen, Pt(x, y-mediumfont->height/2),
			display->black, ZP, mediumfont, label);
}

void
hashmarks(Point p, int dy, int which)
{
	switch(index2which(which)){
	case Mrtt:
		hashmark(p, dy, rttscale(1000000), Rttmax, "1.");
		hashmark(p, dy, rttscale(100000), Rttmax, "0.1");
		hashmark(p, dy, rttscale(10000), Rttmax, "0.01");
		hashmark(p, dy, rttscale(1000), Rttmax, "0.001");
		break;
	case Mlost:
		hashmark(p, dy, 75, 100, " 75%");
		hashmark(p, dy, 50, 100, " 50%");
		hashmark(p, dy, 25, 100, " 25%");
		break;
	}
}

Point
paritypt(int x)
{
	return Pt(x+parity, 0);
}

Point
datapoint(Graph *g, int x, long v, long vmax)
{
	Point p;

	p.x = x;
	p.y = g->r.max.y - Dy(g->r)*v/vmax - Dot;
	if(p.y < g->r.min.y)
		p.y = g->r.min.y;
	if(p.y > g->r.max.y-Dot)
		p.y = g->r.max.y-Dot;
	return p;
}

void
drawdatum(Graph *g, int x, long prev, long v, long vmax)
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
	g->vmax = vmax;
}

void
drawmark(Graph *g, int x)
{
	int c;

	c = (g->colindex+1)&Ncolor;
	draw(screen, Rect(x, g->r.min.y, x+1, g->r.max.y), cols[c][2], nil, ZP);
}

void
redraw(Graph *g, int vmax)
{
	int i, c;

	c = g->colindex;
	draw(screen, g->r, cols[c][0], nil, paritypt(g->r.min.x));
	for(i=1; i<Dx(g->r); i++)
		drawdatum(g, g->r.max.x-i, g->data[i-1], g->data[i], vmax);
	drawdatum(g, g->r.min.x, g->data[i], g->data[i], vmax);
}

void
clearmsg(Graph *g)
{
	if(g->overtmp != nil)
		draw(screen, g->overtmp->r, g->overtmp, nil, g->overtmp->r.min);
	g->overflow = 0;
}

void
drawmsg(Graph *g, char *msg)
{
	if(g->overtmp == nil)
		return;

	/* save previous contents of screen */
	draw(g->overtmp, g->overtmp->r, screen, nil, g->overtmp->r.min);

	/* draw message */
	if(strlen(msg) > g->overtmplen)
		msg[g->overtmplen] = 0;
	string(screen, g->overtmp->r.min, display->black, ZP, mediumfont, msg);
}

void
clearcursor(Graph *g)
{
	int x;
	long prev;

	if(g->overtmp == nil)
		return;

	if(g->cursor > 0 && g->cursor < g->ndata){
		x = g->r.max.x - g->cursor;
		prev = 0;
		if(g->cursor > 0)
			prev = g->data[g->cursor-1];
		drawdatum(g, x, prev, g->data[g->cursor], g->vmax);
		g->cursor = -1;
	}
}

void
drawcursor(Graph *g, int x)
{
	if(g->overtmp == nil)
		return;

	draw(screen, Rect(x, g->r.min.y, x+1, g->r.max.y), cols[g->colindex][2], nil, ZP);
}

void
update1(Graph *g, long v, long vmax, long mark)
{
	char buf[Gmsglen];

	/* put back screen value sans message */
	if(g->overflow || *g->msg){
		clearmsg(g);
		g->overflow = 0;
	}

	draw(screen, g->r, screen, nil, Pt(g->r.min.x+1, g->r.min.y));
	drawdatum(g, g->r.max.x-1, g->data[0], v, vmax);
	if(mark)
		drawmark(g, g->r.max.x-1);
	memmove(g->data+1, g->data, (g->ndata-1)*sizeof(g->data[0]));
	g->data[0] = v;
	if(v>vmax){
		g->overflow = 1;
		sprint(buf, "%ld", v);
		drawmsg(g, buf);
	} else if(*g->msg)
		drawmsg(g, g->msg);

	if(g->cursor >= 0){
		g->cursor++;
		if(g->cursor >= g->ndata){
			g->cursor = -1;
			if(*g->msg){
				clearmsg(g);
				*g->msg = 0;
			}
		}
	}

}

void
pinglost(Machine *m, Req*)
{
	m->lostmsgs++;
}

void
pingreply(Machine *m, Req *r)
{
	ulong x;

	x = r->time/1000LL;
	m->rttsum += x;
	m->rcvdmsgs++;
	m->rttmsgs++;
}


void
pingclean(Machine *m, ushort seq, vlong now, int)
{
	Req **l, *r;
	vlong x, y;

	y = 10LL*1000000000LL;
	for(l = &m->first; *l; ){
		r = *l;
		x = now - r->time;
		if(x > y || r->seq == seq){
			*l = r->next;
			r->time = x;
			if(r->seq != seq)
				pinglost(m, r);
			else
				pingreply(m, r);
			free(r);
		} else
			l = &(r->next);
	}
}

/* IPv4 only */
void
pingsend(Machine *m)
{
	int i;
	char buf[128], err[ERRMAX];
	Icmphdr *ip;
	Req *r;

	ip = (Icmphdr *)(buf + IPV4HDR_LEN);
	memset(buf, 0, sizeof buf);
	r = malloc(sizeof *r);
	if(r == nil)
		return;

	for(i = 32; i < MSGLEN; i++)
		buf[i] = i;
	ip->type = EchoRequest;
	ip->code = 0;
	ip->seq[0] = m->seq;
	ip->seq[1] = m->seq>>8;
	r->seq = m->seq;
	r->next = nil;
	lock(m);
	pingclean(m, -1, nsec(), 0);
	if(m->first == nil)
		m->first = r;
	else
		m->last->next = r;
	m->last = r;
	r->time = nsec();
	unlock(m);
	if(write(m->pingfd, buf, MSGLEN) < MSGLEN){
		errstr(err, sizeof err);
		if(strstr(err, "unreach")||strstr(err, "exceed"))
			m->unreachable++;
	}
	m->seq++;
}

/* IPv4 only */
void
pingrcv(void *arg)
{
	int i, n, fd;
	uchar buf[512];
	ushort x;
	vlong now;
	Icmphdr *ip;
	Ip4hdr *ip4;
	Machine *m = arg;

	ip4 = (Ip4hdr *)buf;
	ip = (Icmphdr *)(buf + IPV4HDR_LEN);
	fd = dup(m->pingfd, -1);
	for(;;){
		n = read(fd, buf, sizeof(buf));
		now = nsec();
		if(n <= 0)
			continue;
		if(n < MSGLEN){
			print("bad len %d/%d\n", n, MSGLEN);
			continue;
		}
		for(i = 32; i < MSGLEN; i++)
			if(buf[i] != (i&0xff))
				continue;
		x = (ip->seq[1]<<8) | ip->seq[0];
		if(ip->type != EchoReply || ip->code != 0)
			continue;
		lock(m);
		pingclean(m, x, now, ip4->ttl);
		unlock(m);
	}
}

void
initmach(Machine *m, char *name)
{
	char *p;

	srand(time(0));
	p = strchr(name, '!');
	if(p){
		p++;
		m->name = estrdup(p+1);
	}else
		p = name;

	m->name = estrdup(p);
	m->nproc = 1;
	m->pingfd = dial(netmkaddr(m->name, "icmp", "1"), 0, 0, 0);
	if(m->pingfd < 0)
		sysfatal("dialing %s: %r", m->name);
	startproc(pingrcv, m);
}

long
rttscale(long x)
{
	if(x == 0)
		return 0;
	x = 10.0*log10(x) - 20.0;
	if(x < 0)
		x = 0;
	return x;
}

double
rttunscale(long x)
{
	double dx;

	x += 20;
	dx = x;
	return pow(10.0, dx/10.0);
}

void
rttval(Machine *m, long *v, long *vmax, long *mark)
{
	ulong x;

	if(m->rttmsgs == 0){
		x = m->lastrtt;
	} else {
		x = m->rttsum/m->rttmsgs;
		m->rttsum = m->rttmsgs = 0;
		m->lastrtt = x;
	}

	*v = rttscale(x);
	*vmax = Rttmax;
	*mark = 0;
}

void
lostval(Machine *m, long *v, long *vmax, long *mark)
{
	ulong x;

	if(m->rcvdmsgs+m->lostmsgs > 0)
		x = (m->lostavg>>1) + (((m->lostmsgs*100)/(m->lostmsgs + m->rcvdmsgs))>>1);
	else
		x = m->lostavg;
	m->lostavg = x;
	m->lostmsgs = m->rcvdmsgs = 0;

	if(m->unreachable){
		m->unreachable = 0;
		*mark = 100;
	} else
		*mark = 0;

	*v = x;
	*vmax = 100;
}

jmp_buf catchalarm;

void
alarmed(void *a, char *s)
{
	if(strcmp(s, "alarm") == 0)
		notejmp(a, catchalarm, 1);
	noted(NDFLT);
}

void
usage(void)
{
	fprint(2, "usage: %s machine [machine...]\n", argv0);
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

int
which2index(int which)
{
	int i, n;

	n = -1;
	for(i=0; i<ngraph; i++){
		if(strcmp(menu2str[which]+Opwid, graph[i].label) == 0){
			n = i;
			break;
		}
	}
	if(n < 0){
		fprint(2, "%s: internal error can't drop graph\n", argv0);
		killall("error");
	}
	return n;
}

int
index2which(int index)
{
	int i, n;

	n = -1;
	for(i=0; i<Nmenu2; i++){
		if(strcmp(menu2str[i]+Opwid, graph[index].label) == 0){
			n = i;
			break;
		}
	}
	if(n < 0){
		fprint(2, "%s: internal error can't identify graph\n", argv0);
		killall("error");
	}
	return n;
}

void
dropgraph(int which)
{
	Graph *ograph;
	int i, j, n;

	if(which > nelem(menu2str))
		abort();
	/* convert n to index in graph table */
	n = which2index(which);
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

void
addmachine(char *name)
{
	if(ngraph > 0){
		fprint(2, "%s: internal error: ngraph>0 in addmachine()\n", argv0);
		usage();
	}
	if(nmach == NMACH)
		sysfatal("too many machines");
	initmach(&mach[nmach++], name);
}


void
resize(void)
{
	int i, j, n, startx, starty, x, y, dx, dy, hashdx, ondata;
	Graph *g;
	Rectangle machr, r;
	long v, vmax, mark;
	char buf[128];

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

	/* label right edge */
	dx = Labspace+stringwidth(mediumfont, "0.001")+Labspace;
	hashdx = dx;
	x = screen->r.max.x - dx;
	y = screen->r.min.y + Labspace+mediumfont->height+Labspace;
	for(i=0; i<ngraph; i++,y+=dy){
		draw(screen, Rect(x, y-1, screen->r.max.x, y), display->black, nil, ZP);
		draw(screen, Rect(x, y, x+dx, screen->r.max.y), cols[graph[i].colindex][0], nil, paritypt(x));
		hashmarks(Pt(x, y), dy, i);
		draw(screen, Rect(x+dx, y, x+dx+1, screen->r.max.y), cols[graph[i].colindex][2], nil, ZP);
	}

	/* label top edge */
	dx = (screen->r.max.x - dx - startx)/nmach;
	for(x=startx, i=0; i<nmach; i++,x+=dx){
		draw(screen, Rect(x-1, starty-1, x, screen->r.max.y), display->black, nil, ZP);
		j = dx/stringwidth(mediumfont, "0");
		n = mach[i].nproc;
		if(n>1 && j>=1+3+(n>10)+(n>100)){	/* first char of name + (n) */
			j -= 3+(n>10)+(n>100);
			if(j <= 0)
				j = 1;
			snprint(buf, sizeof buf, "%.*s(%d)", j, mach[i].name, n);
		}else
			snprint(buf, sizeof buf, "%.*s", j, mach[i].name);
		string(screen, Pt(x+Labspace, screen->r.min.y + Labspace), display->black, ZP,
			mediumfont, buf);
	}
	/* draw last vertical line */
	draw(screen,
		Rect(screen->r.max.x-hashdx-1, starty-1, screen->r.max.x-hashdx, screen->r.max.y),
		display->black, nil, ZP);

	/* create graphs */
	for(i=0; i<nmach; i++){
		machr = Rect(startx+i*dx, starty, screen->r.max.x, screen->r.max.y);
		if(i < nmach-1)
			machr.max.x = startx+(i+1)*dx - 1;
		else
			machr.max.x = screen->r.max.x - hashdx - 1;
		y = starty;
		for(j=0; j<ngraph; j++, y+=dy){
			g = &graph[i*ngraph+j];
			/* allocate data */
			ondata = g->ndata;
			g->ndata = Dx(machr)+1;	/* may be too many if label will be drawn here; so what? */
			g->data = erealloc(g->data, g->ndata*sizeof(long));
			if(g->ndata > ondata)
				memset(g->data+ondata, 0, (g->ndata-ondata)*sizeof(long));
			/* set geometry */
			g->r = machr;
			g->r.min.y = y;
			g->r.max.y = y+dy - 1;
			if(j == ngraph-1)
				g->r.max.y = screen->r.max.y;
			draw(screen, g->r, cols[g->colindex][0], nil, paritypt(g->r.min.x));
			g->overflow = 0;
			*g->msg = 0;
			freeimage(g->overtmp);
			g->overtmp = nil;
			g->overtmplen = 0;
			r = g->r;
			r.max.y = r.min.y+mediumfont->height;
			n = (g->r.max.x - r.min.x)/stringwidth(mediumfont, "9");
			if(n > 4){
				if(n > Gmsglen)
					n = Gmsglen;
				r.max.x = r.min.x+stringwidth(mediumfont, "9")*n;
				g->overtmplen = n;
				g->overtmp = allocimage(display, r, screen->chan, 0, -1);
			}
			g->newvalue(g->mach, &v, &vmax, &mark);
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
		fprint(2, "%s: can't reattach to window\n", argv0);
		killall("reattach");
	}
	resize();
	unlockdisplay(display);
}

void
dobutton2(Mouse *m)
{
	int i;

	for(i=0; i<Nmenu2; i++)
		if(present[i])
			memmove(menu2str[i], "drop ", Opwid);
		else
			memmove(menu2str[i], "add  ", Opwid);
	i = emenuhit(3, m, &menu2);
	if(i >= 0){
		if(!present[i])
			addgraph(i);
		else if(ngraph > 1)
			dropgraph(i);
		resize();
	}
}

void
dobutton1(Mouse *m)
{
	int i, n, dx, dt;
	Graph *g;
	char *e;
	double f;

	for(i = 0; i < ngraph*nmach; i++){
		if(ptinrect(m->xy, graph[i].r))
			break;
	}
	if(i == ngraph*nmach)
		return;

	g = &graph[i];
	if(g->overtmp == nil)
		return;

	/* clear any previous message and cursor */
	if(g->overflow || *g->msg){
		clearmsg(g);
		*g->msg = 0;
		clearcursor(g);
	}

	dx = g->r.max.x - m->xy.x;
	g->cursor = dx;
	dt = dx*pinginterval;
	e = &g->msg[sizeof(g->msg)];
	seprint(g->msg, e, "%s", ctime(starttime-dt/1000)+11);
	g->msg[8] = 0;
	n = 8;

	switch(index2which(i)){
	case Mrtt:
		f = rttunscale(g->data[dx]);
		seprint(g->msg+n, e, " %3.3g", f/1000000);
		break;
	case Mlost:
		seprint(g->msg+n, e, " %ld%%", g->data[dx]);
		break;
	}

	drawmsg(g, g->msg);
	drawcursor(g, m->xy.x);
}

void
mouseproc(void*)
{
	Mouse mouse;

	for(;;){
		mouse = emouse();
		if(mouse.buttons == 4){
			lockdisplay(display);
			dobutton2(&mouse);
			unlockdisplay(display);
		} else if(mouse.buttons == 1){
			lockdisplay(display);
			dobutton1(&mouse);
			unlockdisplay(display);
		}
	}
}

void
startproc(void (*f)(void*), void *arg)
{
	int pid;

	switch(pid = rfork(RFPROC|RFMEM|RFNOWAIT)){
	case -1:
		fprint(2, "%s: fork failed: %r\n", argv0);
		killall("fork failed");
	case 0:
		f(arg);
		killall("process died");
		exits(nil);
	}
	pids[npid++] = pid;
}

void
main(int argc, char *argv[])
{
	int i, j;
	long v, vmax, mark;
	char flags[10], *f, *p;

	fmtinstall('V', eipfmt);

	f = flags;
	pinginterval = 5000;		/* 5 seconds */
	ARGBEGIN{
	case 'i':
		p = ARGF();
		if(p == nil)
			usage();
		pinginterval = atoi(p);
		break;
	default:
		if(f - flags >= sizeof(flags)-1)
			usage();
		*f++ = ARGC();
		break;
	}ARGEND
	*f = 0;

	for(i=0; i<argc; i++)
		addmachine(argv[i]);

	for(f = flags; *f; f++)
		switch(*f){
		case 'l':
			addgraph(Mlost);
			break;
		case 'r':
			addgraph(Mrtt);
			break;
		}

	if(nmach == 0)
		usage();

	if(ngraph == 0)
		addgraph(Mrtt);

	for(i=0; i<nmach; i++)
		for(j=0; j<ngraph; j++)
			graph[i*ngraph+j].mach = &mach[i];

	if(initdraw(nil, nil, argv0) < 0){
		fprint(2, "%s: initdraw failed: %r\n", argv0);
		exits("initdraw");
	}
	colinit();
	einit(Emouse);
	notify(nil);
	startproc(mouseproc, 0);
	display->locking = 1;	/* tell library we're using the display lock */

	resize();

	starttime = time(0);

	unlockdisplay(display); /* display is still locked from initdraw() */
	for(j = 0; ; j++){
		lockdisplay(display);
		if(j == nmach){
			parity = 1-parity;
			j = 0;
			for(i=0; i<nmach*ngraph; i++){
				graph[i].newvalue(graph[i].mach, &v, &vmax, &mark);
				graph[i].update(&graph[i], v, vmax, mark);
			}
			starttime = time(0);
		}
		flushimage(display, 1);
		unlockdisplay(display);
		pingsend(&mach[j%nmach]);
		sleep(pinginterval/nmach);
	}
}
