#include <all.h>

/*
 * See st.c for a description of the data structures.
 *
 * Pos is the time and pixel slice into the time window shown
 * and the rectangle plotted.
 */
typedef struct Pos Pos;

struct Pos
{
	vlong	t0;
	vlong	ival;
	int		p0;
	int		pend;
	vlong	npp;	/* ns per pix at this scale */
};

static char *plumbfname = "ptrace.out";
static char *infname;
int what, verb, newwin;
Biobuf *bout;
static St *(*readst)(Biobuf*);

int mainstacksize = Stack;

static int piddx, graphdy;
static Rectangle plotr;
static Image *im, *bgcol, *rdycol, *waitcol, *runcol;
// These are aliases:
static Image *setcol, *sccol, *pfcol, *faultcol;

static Channel *eventc;
static Mousectl *mousectl;
static Keyboardctl *keyboardctl;
static Proc *current;

static Cursor crosscursor = {
	{-7, -7},
	{0x03, 0xC0, 0x03, 0xC0, 0x03, 0xC0, 0x03, 0xC0,
	 0x03, 0xC0, 0x03, 0xC0, 0xFF, 0xFF, 0xFF, 0xFF,
	 0xFF, 0xFF, 0xFF, 0xFF, 0x03, 0xC0, 0x03, 0xC0,
	 0x03, 0xC0, 0x03, 0xC0, 0x03, 0xC0, 0x03, 0xC0, },
	{0x00, 0x00, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80,
	 0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x7F, 0xFE,
	 0x7F, 0xFE, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80,
	 0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x00, 0x00, }
};

Cursor busycursor ={
	{-1, -1},
	{0xff, 0x80, 0xff, 0x80, 0xff, 0x00, 0xfe, 0x00, 
	 0xff, 0x00, 0xff, 0x80, 0xff, 0xc0, 0xef, 0xe0, 
	 0xc7, 0xf0, 0x03, 0xf0, 0x01, 0xe0, 0x00, 0xc0, 
	 0x03, 0xff, 0x03, 0xff, 0x03, 0xff, 0x03, 0xff, },
	{0x00, 0x00, 0x7f, 0x00, 0x7e, 0x00, 0x7c, 0x00, 
	 0x7e, 0x00, 0x7f, 0x00, 0x6f, 0x80, 0x47, 0xc0, 
	 0x03, 0xe0, 0x01, 0xf0, 0x00, 0xe0, 0x00, 0x40, 
	 0x00, 0x00, 0x01, 0xb6, 0x01, 0xb6, 0x00, 0x00, }
};

Cursor leftcursor = {
	{-8, -7},
	{0x07, 0xc0, 0x04, 0x40, 0x04, 0x40, 0x04, 0x58, 
	 0x04, 0x68, 0x04, 0x6c, 0x04, 0x06, 0x04, 0x02, 
	 0x04, 0x06, 0x04, 0x6c, 0x04, 0x68, 0x04, 0x58, 
	 0x04, 0x40, 0x04, 0x40, 0x04, 0x40, 0x07, 0xc0, },
	{0x00, 0x00, 0x03, 0x80, 0x03, 0x80, 0x03, 0x80, 
	 0x03, 0x90, 0x03, 0x90, 0x03, 0xf8, 0x03, 0xfc, 
	 0x03, 0xf8, 0x03, 0x90, 0x03, 0x90, 0x03, 0x80, 
	 0x03, 0x80, 0x03, 0x80, 0x03, 0x80, 0x00, 0x00, }
};

Cursor rightcursor = {
	{-7, -7},
	{0x03, 0xe0, 0x02, 0x20, 0x02, 0x20, 0x1a, 0x20, 
	 0x16, 0x20, 0x36, 0x20, 0x60, 0x20, 0x40, 0x20, 
	 0x60, 0x20, 0x36, 0x20, 0x16, 0x20, 0x1a, 0x20, 
	 0x02, 0x20, 0x02, 0x20, 0x02, 0x20, 0x03, 0xe0, },
	{0x00, 0x00, 0x01, 0xc0, 0x01, 0xc0, 0x01, 0xc0, 
	 0x09, 0xc0, 0x09, 0xc0, 0x1f, 0xc0, 0x3f, 0xc0, 
	 0x1f, 0xc0, 0x09, 0xc0, 0x09, 0xc0, 0x01, 0xc0, 
	 0x01, 0xc0, 0x01, 0xc0, 0x01, 0xc0, 0x00, 0x00, }
};

static void
busy(void)
{
	setcursor(mousectl, &busycursor);
}

enum{Browsing, Paused};
static void
ready(int state)
{
	if(state == Paused)
		setcursor(mousectl, nil);
	else
		setcursor(mousectl, &crosscursor);
}

static void
clear(void)
{
	if(im != nil)
		if(Dx(screen->r) != Dx(im->r) || Dy(screen->r) != Dy(im->r)){
			freeimage(im);
			im = nil;
		}
	if(im == nil)
			im = allocimage(display, screen->r, screen->chan, 0, -1);
	if(im == nil)
		sysfatal("allocimage: %r");
	draw(im, im->r, bgcol, nil, ZP);
	plotr = im->r;
	plotr.min.x += piddx;
	plotr.min.y += font->height+1;
}

enum{ All, TimeAndPids, JustTime };
static void
flush(int justtime)
{
	Rectangle r;

	r = screen->r;
	if(justtime)
		r.max.y = r.min.y + font->height;
	draw(screen, r, im, nil, im->r.min);
	if(justtime == TimeAndPids){
		r = screen->r;
		r.min.y += font->height;
		r.max.x = r.min.x + piddx;
		draw(screen, r, im, nil, r.min);
	}
	flushimage(display, 1);
}

static int
proclocate(Proc *p, vlong t, int *ip)
{
	int i, id;

	for(i = 0; i < p->nstate; i++){
		id = p->state[i];
		if(graph[id]->time > t)
			break;
	}
	if(i == 0)
		*ip = i;
	else
		*ip = --i;
	return p->state[i];
}

/*
 * Fill up pp, after the user changes Pos.t0 and/or Pos.ival.
 * Locate sets [s0:send] in procs, used later to plot them.
 */
static void
locate(Pos *pp)
{
	int i, pmin, pmax, x;

	pmin = 0;
	pmax = ngraph;
	for(i = 0; i < nproc; i++){
		Proc *p = &proc[i];
		x = proclocate(p, pp->t0, &p->s0);
		if(x < pmin)
			pmin = x;
		x = proclocate(p, pp->t0+pp->ival, &p->send);
		p->send++;
		if(x > pmax)
			pmax = x;
	}
	pp->p0 = pmin;
	pp->pend = pmax+1;
	if(Dx(plotr) > 0)
		pp->npp = pp->ival / (vlong)Dx(plotr);
}

static int infominx, infomaxx;

static void
drawtime(vlong t, vlong ival)
{
	Rectangle r;
	char s[80];
	Point p;

	r = im->r;
	r.max.y = r.min.y + font->height;
	draw(im, r, bgcol, nil, ZP);
	seprint(s, s+sizeof s, "t: %#T", t);
	p = Pt(im->r.max.x - stringwidth(font, s) - 1, r.min.y);
	string(im, p, display->black, ZP, font, s);
	infomaxx = p.x-1;
	infominx = im->r.min.x;
	if(ival > 0){
		seprint(s, s+sizeof s, "ival: %#T", ival);
		infominx = im->r.min.x+1 + stringwidth(font, s)+1;
		if(infomaxx < infominx)
			return;
		p = Pt(im->r.min.x+1, r.min.y);
		string(im, p, display->black, ZP, font, s);
	}
}

static void
drawpid(Proc *pp)
{
	Point p;
	St *g;
	char spid[80];

	p.x = im->r.min.x;
	p.y = im->r.min.y + graphdy * (pp->row+1);
	if(p.y >= im->r.max.y)
		return;
	g = pgraph(pp, pp->s0);
	draw(im, Rpt(p, addpt(p, Pt(piddx, font->height))), bgcol, nil, ZP);
	if(g->pid > 0){
		seprint(spid, spid+sizeof spid, "%-4.4s% udm%ud", g->name, g->pid, g->machno);
		if(pp == current)
			string(im, p, setcol, ZP, font, spid);
		else
			string(im, p, display->black, ZP, font, spid);
	}
}

static void
drawnotice(char *s)
{
	Point p;
	int dx, w, n;
	char buf[80];
	Rectangle r;
	
	dx = infomaxx - infominx;
	r = Rect(infominx, im->r.min.y, infomaxx, im->r.min.y+graphdy);
	draw(im, r, bgcol, nil, ZP);
	w = stringwidth(font, s);
	if(w + 2 > dx){
		seprint(buf, buf+sizeof buf, s);
		s = buf;
		for(n = strlen(s); n > 0; n--){
			w = stringwidth(font, s);
			if(dx > w+2)
				break;
			s[n-1] = 0;
		}
		if(n == 0)
			return;
	}
	p = Pt(infominx + dx/2 - w/2, im->r.min.y);
	string(im, p, setcol, ZP, font, s);
}

static void
drawinfo(St *g)
{
	char s[80];

	seprint(s, s+sizeof s, "%#G", g);
	drawnotice(s);
}

static int
dtwidth(vlong dt, vlong nsperpix)
{
	if(dt < 0)
		return 0;
	if(nsperpix <= 0)
		return Dx(plotr);
	return (int)(dt / nsperpix);
}

/*
 * Try to draw events within the state represented by g
 * ending in eg, if the scale permits.
 */
static void
drawgraphs(St *g, St *eg, Point pt, Pos p)
{
	int x, faults, scs, pfs;
	vlong t, t0, dt;
	Rectangle r;

	x = pt.x;
	t0 = g->time;
	t = t0;
	pfs = faults = scs = 0;
	while(g->pnext != 0 && graph[g->pnext] != eg){
		g = graph[g->pnext];
		g->x = x;
		if(g->etype == STrap)
			if(g->arg&(STrapRPF|STrapWPF))
				pfs++;
			else if(g->arg&STrapSC)
				scs++;
			else
				faults++;
		dt = g->time - t;
		if(dt < p.npp)
			continue;
		if(scs){
			r = Rect(x, pt.y-3*Wid/2, x+Wid, pt.y-Wid/2);
			draw(im, r, sccol, nil, ZP);
		}
		if(faults || pfs){
			r = Rect(x, pt.y+3*Wid/2, x+Wid, pt.y+5*Wid/2);
			draw(im, r, pfs?pfcol:faultcol, nil, ZP);
		}
		faults = pfs = scs = 0;
		t = g->time;
		x = plotr.min.x + dtwidth(t-p.t0, p.npp);
	}
}

static void
drawproc(Proc *pp, Pos p)
{
	St *g, *ng;
	int s;
	Image *c;
	vlong t0, tn, frac;
	int x0, xn;
	Rectangle r;

	frac = 0;
	r = plotr;
	r.min.y += graphdy * pp->row + graphdy/2;
	r.max.y = r.min.y + Wid;
	for(s = pp->s0; s < pp->send; s++){
		g = pgraph(pp, s);
		switch(g->state){
			case SRun:
				c = runcol;
				break;
			case SDead:
				c = bgcol;
				break;
			case SReady:
				c = rdycol;
				break;
			default:
				c = waitcol;
		}
		t0 = g->time - p.t0;
		if(s < pp->nstate-1){
			ng = pgraph(pp, s+1);
			tn = ng->time - p.t0;
		}else
			break;
		x0 = dtwidth(t0, p.npp);
		xn = dtwidth(tn+frac, p.npp);
		g->x = plotr.min.x + x0;
		r.min.x = g->x;
		if(xn == x0){
			frac += tn-t0;
			continue;
		}
		frac = 0;
		if(xn - x0 < Wid)
			xn = x0 + Wid;
		r.max.x = plotr.min.x + xn;
		draw(im, r, c, nil, ZP);
		drawgraphs(g, ng, r.min, p);
	}
	if(s < pp->nstate)
		pgraph(pp, s)->x = r.max.x;
}

/*
 * Plot for the range indicated in pos.
 * Processes must be located before.
 * This updates St.x in the states shown for procs.
 */
static void
plotallat(Pos p)
{
	int i;

	clear();
	for(i = 0; i < nproc; i++){
		drawpid(&proc[i]);
		drawproc(&proc[i], p);
	}
	drawtime(p.t0, p.ival);
}

static Point
ptinplot(Point pt)
{
	if(pt.y < plotr.min.y)
		pt.y = plotr.min.y;
	if(pt.y > plotr.max.y-1)
		pt.y = plotr.max.y-1;
	if(pt.x < plotr.min.x)
		pt.x = plotr.min.x;
	if(pt.x > plotr.max.x-1)
		pt.x = plotr.max.x-1;
	return pt;
}

static int
ptrow(Point pt)
{
	return (pt.y - plotr.min.y) / graphdy;
}

static Proc*
procat(Point pt)
{
	int row, i;
	Proc *pp;

	row = ptrow(pt);
	for(i = 0; i < nproc; i++){
		pp = &proc[i];
		if(pp->s0 < pp->send && pp->row == row)
			return pp;
	}
	return nil;
}

static int
stateat(Proc *pp, Point pt)
{
	int s, last;
	St *g;

	last = pp->s0;
	for(s = pp->s0; s < pp->send; s++){
		g = pgraph(pp, s);
		if(pt.x < g->x)
			break;
		last = s;
	}
	return last;
}

static St*
graphat(Proc *pp, int s, Point pt, vlong npp)
{
	St *g0, *g, *ng, *eg;

	g0 = pgraph(pp, s);

	/* If all's in a pixel, don't bother */
	if(s < pp->send-1){
		eg = pgraph(pp, s+1);
		if(eg->time - g0->time < npp)
			return g0;
	}else
		eg = nil;
	for(g = g0; g->pnext != 0 && graph[g->pnext] != eg; g = ng){
		ng = graph[g->pnext];
		if(ng->x > pt.x)
			break;
	}
	return g;
}

static void
tplumb(vlong t)
{
	static int fd = -1;
	Plumbmsg *pm;
	static char wd[128];
	Plumbattr *attr;

	if(fd < 0)
		fd = plumbopen("send", OWRITE);
	if(fd < 0){
		fprint(2, "%s: plumbopen: %r\n", argv0);
		return;
	}
	if(wd[0] == 0)
		getwd(wd, sizeof wd);

	pm = mallocz(sizeof *pm, 1);
	attr = mallocz(sizeof *attr, 1);
	if(pm == nil || attr == nil){
		free(pm);
		fprint(2, "%s: no memory\n", argv0);
		return;
	}
	pm->src = strdup(argv0);
	pm->dst = strdup("edit");
	pm->wdir = strdup(wd);
	pm->type = strdup("text");
	pm->data = strdup(plumbfname);
	pm->ndata = strlen(pm->data);
	attr->name = strdup("addr");
	attr->value = smprint("/^%T .*\\n", t);

	pm->attr = plumbaddattr(pm->attr, attr);
	plumbsend(fd, pm);
}

static void
cursorat(Point pt, int doplumb, Pos p)
{
	Proc *pp, *old;
	St *g;
	int gi, dx;

	pt = ptinplot(pt);
	pp = procat(pt);
	if(pp == nil)
		return;
	gi = stateat(pp, pt);
	g = graphat(pp, gi, pt, p.npp);
	
	dx = pt.x - plotr.min.x;
	drawtime(p.t0 + dx * p.npp, p.ival);
	drawinfo(g);
	old = current;
	current = pp;
	if(old != nil)
		drawpid(old);
	drawpid(current);
	flush(TimeAndPids);
	if(doplumb)
		tplumb(g->time);
}

static void
printproc(Biobuf *b, Proc *pp, vlong t0, vlong ival)
{
	St *g0, *ge, *g;
	vlong tend;

	if(t0 < 0 || ival < 0){
		t0 = 0;
		tend = graph[ngraph-1]->time + 1;
	}else
		tend = t0+ival;
	ge = pgraph(pp, pp->send-1);
	g0 = pgraph(pp, pp->s0);

	for(g = g0; g != ge; g = graph[g->pnext])
		if(g->time >= t0 && g->time < tend)
			Bprint(b, "%G\n", g);
}

static void
printallat(Biobuf *b, vlong t0, vlong ival, int rmin, int rend)
{
	Biobuf *bout;
	int i;

	if(rmin < 0 || rend < 0){
		rmin = 0;
		rend = nproc;
	}else if(rend > nproc)
		rend = nproc;
		
	if(b != nil)
		bout = b;
	else{
		bout = Bopen("ptrace.out", OWRITE);
		if(bout == nil){
			drawnotice("write failed");
			flush(JustTime);
			return;
		}
	}
	for(i = rmin; i < rend; i++)
		printproc(bout, &proc[i], t0, ival);

	Bprint(bout, "\n");

	if(b == nil){
		drawnotice("written ptrace.out");
		flush(JustTime);
		Bterm(bout);
	}else
		Bflush(b);
}

static void
flagsweep(Point p0)
{
	line(screen, Pt(p0.x, p0.y-8), Pt(p0.x, p0.y+8), 
		ARROW(4, 4, 2), ARROW(4, 4, 2), 1, display->black, ZP);
	line(screen, Pt(p0.x-10, p0.y), Pt(p0.x+10, p0.y), 
		ARROW(4, 4, 2), ARROW(4, 4, 2), 1, display->black, ZP);
	flushimage(display, 1);
}

static void
sweep(int b, Point p0, Pos *p)
{
	enum{None, Left, Right};
	int where, rmin, rend;
	Point pt;
	Rectangle r;
	Mouse m;
	Pos np;

	p0 = ptinplot(p0);
	flagsweep(p0);
	where = None;
	do{
		recv(mousectl->c, &m);
		pt = ptinplot(m.xy);
		if(pt.x > p0.x && where != Right){
			setcursor(mousectl, &rightcursor);
			where = Right;
		}else if(pt.x <= p0.x && where != Left){
			setcursor(mousectl, &leftcursor);
			where = Left;
		}
		r = canonrect(Rpt(p0, pt));
	}while(m.buttons);

	np = *p;
	np.t0 += (r.min.x-plotr.min.x) * np.npp;
	if(np.t0 > graph[ngraph-1]->time)
		np.t0 = graph[ngraph-1]->time;

	np.ival = np.npp * Dx(r);
	if(np.ival < US(1))
		np.ival = US(1);
	if(np.ival > graph[ngraph-1]->time + US(1))
		np.ival = graph[ngraph-1]->time + US(1);

	rmin = ptrow(r.min);
	rend = ptrow(r.max) + 1;
	switch(b){
	case 1:
		*p = np;
		locate(p);
		break;
	case 2:
		printallat(nil, np.t0, np.ival, rmin, rend);
		break;
	case 4:
		printallat(bout, np.t0, np.ival, rmin, rend);
		break;
	}
}

static void
setrows(void)
{
	int i;

	for(i = 0; i < nproc; i++)
		proc[i].row = i;
}

static void
debugdump(Pos p)
{
	Proc *pp;
	int i;

	Bprint(bout, "pos p0 %d pend %d t0 %T ival %T\n",
		p.p0, p.pend, p.t0, p.ival);
	for(i = 0; i < nproc; i++){
		pp = &proc[i];
		Bprint(bout, "proc %d pid %d s0 %d send %d ns %d\n",
			i, pp->pid, pp->s0, pp->send, pp->nstate);
	}
	// XXX
	pp = &proc[13];
	Bprint(bout, "%R\n", plotr);
	for(i = pp->s0; i < pp->send; i++)
		Bprint(bout, "x %d %G\n", pgraph(pp, i)->x, pgraph(pp, i));
	Bflush(bout);
}

static void
browse(void)
{
	Rune r;
	Mouse m;
	Pos p;
	vlong tmax;
	int paused, b;
	Point lastxy;
	enum{Ptr, Rsz, Kbd};
	Alt a[] = {
		{mousectl->c, &m, CHANRCV},
		{mousectl->resizec, nil, CHANRCV},
		{keyboardctl->c, &r, CHANRCV},
		{nil, nil, CHANEND}
	};


	setrows();
restart:
	p.p0 = 0;
	p.pend = ngraph;
	p.ival = graph[ngraph-1]->time + US(1);
	p.t0 = 0;
	busy();
	clear();	/* set plotr; locate uses that */
	locate(&p);
	tmax = p.ival;
	paused = Browsing;
	plotallat(p);
	flush(All);
	for(;;){
		ready(paused);
		switch(alt(a)){
		case Rsz:
			if(getwindow(display, Refnone) < 0)
				sysfatal("getwindow: %r");
			if(im != nil)
				freeimage(im);
			im = nil;
		redraw:
			busy();
			plotallat(p);
			flush(All);
			while(nbrecv(keyboardctl->c, &r) != 0)
				;
			break;
		case Ptr:
			b = 0;
			if(m.buttons){
				b = m.buttons;
				lastxy = m.xy;
				recv(mousectl->c, &m);
				if(m.buttons == 0){
					if(b == 1 && lastxy.y < plotr.min.y)
						if(lastxy.x < plotr.min.x + Dx(plotr))
							goto right;
						else
							goto left;
				}else {
					busy();
					sweep(b, lastxy, &p);
					goto redraw;
				}
			}else
				while(nbrecv(mousectl->c, &m) != 0)
					;
			if(paused)
				break;

			if(ptinrect(m.xy, plotr) && (b==4 || !eqpt(lastxy, m.xy))){
				cursorat(m.xy, b==4, p);
				lastxy = m.xy;
			}
			break;
		case Kbd:
			switch(r){
			case 'D':
				debugdump(p);
				break;
			case Kesc:
				busy();
				goto restart;
			case ' ':
				paused = !paused;
				break;
			case 'q':
				Bterm(bout);
				threadexitsall(nil);
			case '+':
			case Kup:
				busy();
				p.ival /= 2;
				if(p.ival < US(1))
					p.ival = US(1);
				locate(&p);
				goto redraw;
			case '-':
			case Kdown:
				if(p.ival > tmax)
					break;
				busy();
				p.ival *= 2;
				locate(&p);
				goto redraw;
			case Kright:
			right:
				busy();
				p.t0 += p.ival/Scroll;
				if(p.t0 > graph[ngraph-1]->time)
					p.t0 = graph[ngraph-1]->time;
				locate(&p);
				goto redraw;
				break;
			case Kleft:
			left:
				if(p.t0 < p.ival/Scroll)
					break;
				busy();
				p.t0 -= p.ival/Scroll;
				locate(&p);
				goto redraw;
				break;
			case 's':
				busy();
				printallat(nil, -1, -1, -1, -1);
				break;
			case 'p':
				busy();
				printallat(bout, -1, -1, -1, -1);
				break;
			}
			break;
		default:
			sysfatal("alt: %r");
		}
	}
}

static void
mkwin(int dx, int dy)
{
	char *wsys, line[256];
	int wfd;

	if((wsys = getenv("wsys")) == nil)
		sysfatal("no wsys: %r");
	if((wfd = open(wsys, ORDWR)) < 0)
		sysfatal("open wsys: %r");
	snprint(line, sizeof(line), "new -pid %d -dx %d -dy %d",
			getpid(), dx, dy);
	line[sizeof(line) - 1] = '\0';
	rfork(RFNAMEG);
	if(mount(wfd, -1, "/mnt/wsys", MREPL, line) < 0) 
		sysfatal("mount wsys: %r");
	if(bind("/mnt/wsys", "/dev", MBEFORE) < 0) 
		sysfatal("mount wsys: %r");
}

static void
colors(void)
{
	piddx = stringwidth(font, "XXXXX" "XXXXXmXX");	/* 5 for name, 5 for pid */
	graphdy = font->height + 1;
	bgcol = allocimagemix(display, DPalebluegreen, DWhite);
	rdycol = allocimage(display, Rect(0,0,1,1), CMAP8, 1, DGreen);
	waitcol = allocimage(display, Rect(0,0,1,1), CMAP8, 1, DRed);
	runcol = allocimage(display, Rect(0,0,1,1), CMAP8, 1, DBlue);
	if(bgcol == nil || rdycol == nil || waitcol == nil || runcol == nil)
		sysfatal("color: %r");
	setcol = runcol;
	sccol = runcol;
	pfcol = waitcol;
	faultcol = display->black;
}

static void
usage(void)
{
	fprint(2, "Usage: %s [-dgpw] file\n", argv0);
	exits(nil);
}

void
threadmain(int argc, char **argv)
{
	int isdev, i;

	isdev = 0;

	ARGBEGIN {
	case 'd':
		isdev = 1;
		break;
	case 'g':
		what |= Plot;
		break;
	case 'p':
		what |= Dump;
		break;
	case 'v':
		verb++;
		break;
	case 'w':
		newwin++;
		break;
	default:
		usage();
	}
	ARGEND;

	if(argc != 1)
		usage();
	infname = argv[0];
	if(!isdev)
		plumbfname = argv[0];
	if(what == 0)
		what = Dump;

	bout = Bopen("/fd/1", OWRITE);
	if(bout == nil)
		sysfatal("stdout: Bopen: %r");

	fmtinstall('T', Tfmt);
	fmtinstall('G', Gfmt);

	readall(infname, isdev);
	if(ngraph == 0)
		sysfatal("nothing to plot");

	makeprocs();

	if(what&Dump)
		for(i = 0; i < ngraph; i++)
			if(Bprint(bout, "%G\n", graph[i]) < 0)
				sysfatal("out: %r");

	if((what&Plot) == 0){
		Bterm(bout);
		exits(nil);
	}

	if(Bflush(bout) < 0)
		sysfatal("out: %r");
	eventc = chancreate(sizeof(ulong), 10);
	if(eventc == nil)
		sysfatal("no memory");
	if(newwin)
		mkwin(Wx, Wy);
	if(initdraw(nil, "/lib/font/bit/pelm/unicode.8.font", argv0) < 0)
		if(initdraw(nil, nil, argv0) < 0)
			sysfatal("initdraw: %r");
	colors();
	if((mousectl = initmouse(nil, screen)) == nil)
		sysfatal("mouse: %r");
	if((keyboardctl = initkeyboard(nil)) == nil)
		sysfatal("keyboard: %r");
	browse();
	Bterm(bout);
}
