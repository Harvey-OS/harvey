#include <all.h>

/*
 * canonicalize lat,lng
 */
void
norm(Loc *l, double lat, double lng)
{

	l->lat = lat * Perdegree;
	l->lng = -lng * Perdegree;
	normal(l);
}

void
normal(Loc *l)
{
	long a, d;

	d = 360*Perdegree;
	a = l->lat;
	while(a < 0)
		a += d;
	while(a >= d)
		a -= d;
	if(a >= 180*Perdegree) {
		if(a < 270*Perdegree) {
			a = 540*Perdegree - a;
			l->lng += 180*Perdegree;
		}
	} else
		if(a >= 90*Perdegree) {
			a = 180*Perdegree - a;
			l->lng += 180*Perdegree;
		}
	l->lat = a;

	a = l->lng;
	while(a < 0)
		a += d;
	while(a >= d)
		a -= d;
	l->lng = a;
}

void
panic(char *s)
{
	print("die: %s\n", s);
	abort();
}

void
initloc(Loc *l)
{
	char buf[100], *p;
	int f;
	double lat, lng;

	f = open("/lib/sky/here", OREAD);
	if(f < 0)
		goto deflt;
	read(f, buf, sizeof(buf));
	close(f);
	lat = atof(buf);
	p = strchr(buf, ' ');
	if(p == 0)
		goto deflt;
	lng = atof(p);
	goto conv;

deflt:
	/*
	 * location of the Unix room
	 */
	lat = 40 + 41.06/60;
	lng = 74 + 23.98/60;

conv:
	norm(l, lat, lng);
}


/*
 * internal malloc.
 * check errors and stores in list
 */
void*
ca(long n)
{
	void *v;
	Mem *m;

	if(map.memleft < n) {
		map.memleft = (1<<12);
		map.memptr = malloc(map.memleft);
		memset(map.memptr, 0, map.memleft);
		map.memtotal1 += map.memleft;
		if(n >= map.memleft)
			panic("ca too big");
		m = ca(sizeof(Mem));
		m->link = map.membase;
		map.membase = m;
	}

	n = (n + 3) & ~3;
	v = map.memptr;
	map.memptr += n;
	map.memleft -= n;
	map.memtotal2 += n;
	return v;
}

void*
memchain(void)
{
	void *v;

	v = map.membase;
	map.membase = 0;
	map.memleft = 0;
	map.memptr = 0;
	return v;
}

void
memfree(void *v)
{
	Mem *m, *m1;

	for(m=v; m; m=m1) {
		m1 = m->link;
		free(m);
	}
}

int
segvis(int ox, int oy, int x, int y, Rec *r)
{

	if(ox < r->min.x && x < r->min.x)
		return 0;

	if(ox >= r->max.x && x >= r->max.x)
		return 0;

	if(oy < r->min.y && y < r->min.y)
		return 0;

	if(oy >= r->max.y && y >= r->max.y)
		return 0;

	if(ox == x && oy == y)
		return 0;

	return 1;

}

int
pointvis(int x, int y, Rec *r)
{

	if(x < r->min.x || x >=r->max.x)
		return 0;

	if(y < r->min.y || y >= r->max.y)
		return 0;

	return 1;
}

long
dtoline(long x1, long y1, long x2, long y2, long x, long y)
{
	long dx1, dx2, dy1, dy2;
	long d1, d2, ix, iy;

	dx1 = x-x1;
	dx2 = x-x2;
	dy1 = y-y1;
	dy2 = y-y2;

	/*
	 * divide along axis crossings
	 */
	if((dx1 > 0 && dx2 < 0) || (dx2 > 0 && dx1 < 0) ||
	   (dy1 > 0 && dy2 < 0) || (dy2 > 0 && dy1 < 0)) {
		iy = (y1 + y2)/2;
		ix = (x1 + x2)/2;
		d1 = dtoline(x1, y1, ix, iy, x, y);
		d2 = dtoline(ix, iy, x2, y2, x, y);
		goto ret;
	}

	if(dx1 < 0)
		dx1 = -dx1;
	if(dx2 < 0)
		dx2 = -dx2;
	if(dy1 < 0)
		dy1 = -dy1;
	if(dy2 < 0)
		dy2 = -dy2;

	/*
	 * in same quadrant
	 * find aprox to nearest point
	 */
	if(dx1 > dy1)
		d1 = dx1 + dy1/2;
	else
		d1 = dy1 + dx1/2;
	if(dx2 > dy2)
		d2 = dx2 + dy2/2;
	else
		d2 = dy2 + dx2/2;

ret:
	if(d1 < d2)
		return d1;
	return d2;
}

char*
convcmd(void)
{
	static char buf[sizeof(cmd)];
	char *bp;
	int i;

	bp = buf;
	for(i=0; cmd[i]; i++)
		bp += runetochar(bp, cmd+i);
	*bp = 0;
	strcpy((char*)cmd, buf);
	return (char*)buf;
}

Loc
invmap(int but)
{
	long x, y;
	Loc loc;

	cursorswitch(&bullseye);
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

	x = ev.mouse.xy.x;
	y = ev.mouse.xy.y;

	/*
	 * distance to center
	 */
	x -= (map.screenr.max.x + map.screenr.min.x)/2;
	y -= (map.screenr.max.y + map.screenr.min.y)/2;

	/*
	 * convert to lat/lng
	 */
	loc.lng = map.here.lng + x/map.xscale;
	loc.lat = map.here.lat + y/map.yscale;
	normal(&loc);

	cursorswitch(0);
	return loc;
}

int
myatol(char *p)
{
	int c, n;

	n = 0;
	c = *p++;
	while(c == ' ')
		c = *p++;
	while(c >= '0' && c <= '9') {
		n = n*10 + (c - '0');
		c = *p++;
	}
	return n;
}
	
Loc
adjloc(void)
{
	Loc loc;
	long d;
	int xflag, yflag;

	loc = map.here;

	xflag = 0;
	d = (map.here.lng - map.loc.lng)*map.xscale;
	if(d < 0)
		d = -d;
	if(d >= map.width/6) {
		xflag = 1;
		if(d > map.width/3)
			xflag = 2;
	}

	yflag = 0;
	d = (map.here.lat - map.loc.lat)*map.yscale;
	if(d < 0)
		d = -d;
	if(d >= map.height/6) {
		yflag = 1;
		if(d > map.height/3)
			yflag = 2;
	}

	/*
	 * if the map has moved off screen in
	 * a single direction, shift the map
	 * in that direction. this will speed up
	 * the drawing by saving 2/3 of the off
	 * screen bitmap.
	 */
	if(xflag == 1 && yflag == 0) {
		loc.lat = map.loc.lat;
		if(map.here.lng > map.loc.lng)
			loc.lng += (map.width/6 - 1)/map.xscale;
		else
			loc.lng -= (map.width/6 - 1)/map.xscale;
	}

	if(xflag == 0 && yflag == 1) {
		loc.lng = map.loc.lng;
		if(map.here.lat > map.loc.lat)
			loc.lat -= (map.height/6 - 1)/map.yscale;
		else
			loc.lat += (map.height/6 - 1)/map.yscale;
	}

	return loc;
}

void
doexit(void)
{

	bitclr(&screen);
	exits(0);
}

void
bitclr(Bitmap *b)
{
	bitblt(b, b->r.min, b, b->r, 0);
}

void
getfiles(char *cmd)
{
	long maxbx, minbx, maxby, minby, perbox;
	int x, y, s, w;
	double xoffset, yoffset, xscale, yscale, r;

	r = atof(cmd) *  map.logsca[6];
	if(r < map.logsca[6])
		r = map.logsca[6];
	if(r > map.logsca[nelem(map.logsca)-1])
		r = map.logsca[nelem(map.logsca)-1];

	for(s=1; s<=25; s*=5) {
		if(s == 1)
			w = 4;
		else
		if(s == 5)
			w = 3;
		else
			w = 2;

		perbox = Perbox * s;
		xscale = 1.0e-3 / r;
		yscale = -1.0e-3 / r /
			cos(map.loc.lat/(57.29578*Perdegree));
		xoffset = map.width/2. - map.loc.lng*xscale;
		yoffset = map.height/2. - map.loc.lat*yscale;

		minbx = (0 - xoffset) / xscale;
		maxbx = (map.width - xoffset) / xscale;
		minby = (map.height - yoffset) / yscale;
		maxby = (0 - yoffset) / yscale;

		minbx = minbx / perbox;
		maxbx = maxbx / perbox;
		minby = minby / perbox;
		maxby = maxby / perbox;

		for(y=minby; y<=maxby; y++) {
			print("mkdir %.*d\n", w, y);
			for(x=minbx; x<=maxbx; x++)
				print("cp /lib/roads/%.*d/%.*d%.*d.h %.*d/%.*d%.*d.h\n",
					w, y, w, y, w, x, w, y, w, y, w, x);
		}
	}
}
