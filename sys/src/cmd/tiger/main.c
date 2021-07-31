#include	<all.h>

void	drawpage(Page*);

void
main(int argc, char *argv[])
{
	int i;
	ulong etype;
	Loc loc;

	binit(0, 0, "tiger");
	einit(Emouse|Ekeyboard);
	etimer(Etimer, 500);

	rscale = (Nscale-1)/2;
	logsca[(Nscale+1)/2] = sqrt(sqrt(10));	/* 4 scales per power of 10 */
	for(i=0; i<Nscale; i++) {
		if(i > (Nscale+1)/2)
			logsca[i] = logsca[i-1]*logsca[(Nscale+1)/2];
		if(i < (Nscale+1)/2)
			logsca[(Nscale+1)/2-i-1] =
				logsca[(Nscale+1)/2-i]/logsca[(Nscale+1)/2];
	}

	ARGBEGIN {
	case 'r':
	case 's':
		rscale = atol(ARGF());
		break;
	} ARGEND

	setdclass();
	normscales();
	initloc(&map.here);

	cursorswitch(&reading);
	for(i=0; i<argc; i++)
		readdict(argv[i], &map.here, 0);

	map.loc = map.here;
	w.loc = map.here;
	sprint(w.p1.line[0], "X99; init");
	for(i=1; i<Nline; i++)
		w.p1.line[i][0] = 0;

	logoinit();
	map.moving = 0;
	ereshaped(screen.r);

	for(;;) {
		etype = eread(Etimer|Emouse|Ekeyboard, &ev);
		while(etype & Etimer) {
			break;
		}
		while(etype & Emouse) {
			if(ev.mouse.buttons & But1) {
				loc = invmap(But1);
				lookup(&loc, 1);
				drawmap();
				cursorswitch(0);
				break;
			}
			if(ev.mouse.buttons & But2) {
				loc = invmap(But2);
				lookup(&loc, 2);
				drawmap();
				cursorswitch(0);
				break;
			}
			if(ev.mouse.buttons & But3) {
				map.loc = invmap(But3);
				map.moving = 0;
				lookup(&map.loc, 1);
				ereshaped(screen.r);
				break;
			}
			break;
		}
		while(etype & Ekeyboard) {
			i = ev.kbdc;
			switch(i) {
			default:
				if(cmdi < sizeof(cmd))
					cmd[cmdi++] = i;
				break;

			case '\n':
				cmd[cmdi] = 0;
				docmd();
				cmdi = 0;
				break;

			case '\b':
				if(cmdi > 0)
					cmdi--;
				break;
			}
			cmd[cmdi] = 0;
			sprint(w.p1.line[3], "%S", cmd);
			drawpage(&w.p1);
			break;
		}
	}
}

void
initloc(Loc *l)
{
	magvar = 12.9;
	l->lat = 40 + 41.06/60;		/* enter your favorite lat/lng here */
	l->lng = 74 + 23.98/60;

	l->lat = norm(l->lat/DEGREE(1), TWOPI);
	l->lng = norm(-l->lng/DEGREE(1), TWOPI);

	l->sinlat = sin(l->lat);
	l->coslat = cos(l->lat);
}

void
drawpage(Page *pg)
{
	int i;
	Point pt;

	bitblt(pg->bit, pg->bit->r.min, pg->bit, pg->bit->r, F);
	pt.x = 2*Inset;
	pt.y = font->height/2;
	for(i=0; i<Nline; i++) {
		string(pg->bit, pt, font, pg->line[i], S^D);
		pt.y += font->height;
	}
	bitblt(&screen, pg->loc, pg->bit, pg->bit->r, S);
}

void
drawmap(void)
{
	Box r;

loop:
	if(map.moving)
		mapcoord(&map.here);
	else
		mapcoord(&map.loc);

	r.min.x = map.x - map.width/3 + Inset;
	r.min.y = map.y - map.height/3 + Inset;
	r.max.x = r.min.x + map.width*2/3 - 2*Inset;
	r.max.y = r.min.y + map.height*2/3 - 2*Inset;

	if(map.moving) {
		if(r.min.x < 0 || r.min.y < 0 ||
		   r.max.x >= map.width || r.max.y >= map.height) {
			map.loc = map.here;
			ereshaped(screen.r);
			goto loop;
		}
	}

	map.offx = map.x;
	map.offy = map.y;

	drawpage(&w.p1);

	bitblt(&screen,
		Pt(w.p1.loc.x, w.p1.loc.y + w.p1.bit->r.max.y),
		map.off,
		Rect(r.min.x, r.min.y+w.p1.bit->r.max.y,
			r.max.x, r.max.y),
		S);

	bflush();
}

void
ereshaped(Box r)
{
	int x, y;
	double d, minlat, maxlat, minlng, maxlng;

	cursorswitch(&thinking);
	screen.r = r;
	x = r.max.x - r.min.x;
	map.width = 3*x/2;
	if(x <= 0)
		x = 1;
	map.scale = RSCALE(x);

	y = r.max.y - r.min.y;
	if(y <= 0)
		y = 1;
	map.height = 3*y/2;

	if(map.off)
		bfree(map.off);
	map.off = balloc(Rect(0,0,map.width,map.height), 1);
	if(map.off == 0)
		exits("balloc map");

	d = (map.width * map.scale)/(2*RADE);
	minlng = map.loc.lng - d;
	maxlng = map.loc.lng + d;
	d = (map.height * map.scale)/(2*RADE);
	minlat = map.loc.lat - d;
	maxlat = map.loc.lat + d;

	drawlist(minlng, maxlng, minlat, maxlat);

	mapcoord(&map.loc);
	if(map.x != 0)
		bitblt(map.off, Pt(map.x-Msize/2, map.y-Msize/2),
			map.poi, map.poi->r, DorS);

	w.p1.loc.x = r.min.x + Inset;
	w.p1.loc.y = r.min.y + Inset;
	if(w.p1.bit)
		bfree(w.p1.bit);
	w.p1.bit = balloc(Rect(0,0, 2*map.width/3 - 2*Inset, (Nline+1)*font->height), 0);

	drawmap();
	cursorswitch(0);
}

void
poilogo(Bitmap *b)
{
	int t;

	t = Msize/2;
	segment(b, Pt(t-3,t), Pt(t+3+1,t), ~0, DorS);
	segment(b, Pt(t,t-3), Pt(t,t+3+1), ~0, DorS);
}

void
logoinit(void)
{

	map.poi = balloc(Rect(0,0, Msize,Msize), 0);
	if(map.poi == 0)
		exits("balloc poi");
	poilogo(map.poi);
}

void
normscales(void)
{
	if(rscale < 0)
		rscale = 0;
	if(rscale >= Nscale)
		rscale = Nscale-1;
}

void
docmd(void)
{
	int c, n, i, flag;
	char *cmd;

	cmd = convcmd();
	flag = 0;
	while(*cmd == ' ')
		cmd++;
	c = *cmd++;
	while(*cmd == ' ')
		cmd++;
	switch(c) {
	default:
		return;

	case 'q':
	case 0x04:
		exits(0);

	case 'r':	/* read-add */
	case 'e':	/* read-clr-add */
		cursorswitch(&reading);
		while(*cmd == ' ')
			cmd++;
		readdict(cmd, &map.loc, c=='e');
		w.loc = map.loc;
		sprint(w.p1.line[0], "X99; init");
		for(i=1; i<Nline; i++)
			w.p1.line[i][0] = 0;
		break;

	case 'p':
		getplace(cmd, &map.loc);
		w.loc = map.loc;
		break;

	case 'l':
		while(*cmd == ' ')
			cmd++;
		if(*cmd) {
			getplace(cmd, &map.loc);
			w.loc = map.loc;
		}
		cursorswitch(&reading);
		while(*cmd == ' ')
			cmd++;
		i = DEGREE(map.loc.lat)*5;
		n = DEGREE(map.loc.lng)*5;
		sprint(cmd, "/lib/roads/%.4d/%.4d.%.4d.h", i, i, n);
		readdict(cmd, &map.loc, 0);
		w.loc = map.loc;
		sprint(w.p1.line[0], "X99; init");
		for(i=1; i<Nline; i++)
			w.p1.line[i][0] = 0;
		break;

	case 's':	/* scale */
		c = *cmd++;
		if(c == '+' || c == '-') {
			flag = c;
			c = *cmd++;
		}
		n = 1;
		if(c >= '0' && c <= '9') {
			n = 0;
			for(;;) {
				n = n*10 + (c-'0');
				c = *cmd++;
				if(c < '0' || c > '9')
					break;
			}
		}
		if(flag == '+')
			n = rscale+n;
		if(flag == '-')
			n = rscale-n;
		rscale = n;
		normscales();
		break;

	case '/':
		cursorswitch(&thinking);
		search(cmd);
		drawmap();
		cursorswitch(0);
		return;

	case 'm':	/* map center */
		c = *cmd;
		if(c == 0) {
			map.moving = !map.moving;
			break;
		}
/*
		r = ref2;
		for(i=0; i<nref2; i++,r++) {
			getname(r, r->type, name);
			if(strcmp(cmd, name) == 0)
				goto found;
		}
		r = ref1;
		for(i=0; i<nref1; i++,r++) {
			getname(r, 'A', name);
			if(strcmp(cmd, name) == 0)
				goto found;
		}
*/
		return;

	found:
/*
		memmove(w.name, name, sizeof(w.name));
		w.loc = r->loc;
		map.loc = r->loc;
		map.moving = 0;
*/
		break;
	}
	ereshaped(screen.r);
}

void
mapcoord(Loc *l)
{
	double d, a;
	int x, y;

	d = dist(&map.loc, l);
	a = angle(&map.loc, l);
	d = d * RADE / map.scale;	/* now in pix on hidden map*/

	x = floor(d*sin(a) + .5);
	map.x = map.width/2 + x;
	if(map.x <= 0 || map.x >= map.width-1)
		goto bad;

	y = floor(d*cos(a) + .5);
	map.y = map.height/2 - y;
	if(map.y <= 0 || map.y >= map.height-1)
		goto bad;

	return;

bad:
	map.x = 0;
	map.y = 0;
}

Loc
invmap(int but)
{
	double d, a;
	double x, y;
	Loc loc;

	cursorswitch(&bullseye);	/* reset by caller */
	for(;;) {
		if(ev.mouse.buttons & but)
			break;
		eread(Emouse, &ev);
	}
	for(;;) {
		if(!(ev.mouse.buttons & but))
			break;
		eread(Emouse, &ev);
	}

	x = +((ev.mouse.xy.x - screen.r.min.x) + map.offx
		- 5*map.width/6);
	y = -((ev.mouse.xy.y - screen.r.min.y) + map.offy
		- 5*map.height/6);
/*
print("xy = %d,%d\n", (int)x, (int)y);
print("dm = %d,%d\n", (ev.mouse.xy.x - screen.r.min.x), (ev.mouse.xy.y - screen.r.min.y));
print("of = %d,%d\n", map.offx, map.offy);
print("wh = %d,%d\n", map.width, map.height);
*/

	if(x == 0 && y == 0)
		return map.loc;

	d = sqrt(x*x + y*y) * map.scale;
	a = atan2(x, y);
	a = norm(a, TWOPI);

	loc = dmecalc(&map.loc, a, d);

	return loc;
}
