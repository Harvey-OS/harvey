#include	<all.h>

void
drawoff(void)
{
	Box *b;
	Record *r;
	Line *l;
	long nx, ny, ox, oy, z;
	Rec t;

	cursorswitch(&drawing);

	t = map.off->r;
	if(map.last.lng == map.loc.lng) {
		ny = (map.last.lat - map.loc.lat) * map.yscale;
		if(ny > 0 && ny < map.height/2) {
			bitblt(map.off,
				Pt(map.off->r.min.x, map.off->r.min.y+ny),
				map.off,
				map.off->r,
				S);
			t.max.y = t.min.y + ny;
			goto redraw;
		}
		if(ny < 0 && ny > -map.height/2) {
			bitblt(map.off,
				Pt(map.off->r.min.x, map.off->r.min.y+ny),
				map.off,
				map.off->r,
				S);
			t.min.y = t.max.y + ny;
			goto redraw;
		}
	}
	if(map.last.lat == map.loc.lat) {
		nx = (map.last.lng - map.loc.lng) * map.xscale;
		if(nx > 0 && nx < map.width/2) {
			bitblt(map.off,
				Pt(map.off->r.min.x+nx, map.off->r.min.y),
				map.off,
				map.off->r,
				S);
			t.max.x = t.min.x + nx;
			goto redraw;
		}
		if(nx < 0 && nx < -map.width/2) {
			bitblt(map.off,
				Pt(map.off->r.min.x+nx, map.off->r.min.y),
				map.off,
				map.off->r,
				S);
			t.min.x = t.max.x + nx;
			goto redraw;
		}
	}

redraw:
	bitblt(map.off,
		t.min,
		map.off,
		t,
		0);

	for(b=map.boxlist; b; b=b->link) {
		for(r=b->lines; r; r=r->link) {
			l = r->line;
			if(l == 0)
				continue;

			z = ~r->grey;
			ox = l->lng*map.xscale + map.xoffset;
			oy = l->lat*map.yscale + map.yoffset;
			for(l=l->link; l; l=l->link) {
				nx = l->lng*map.xscale + map.xoffset;
				ny = l->lat*map.yscale + map.yoffset;

				if(segvis(ox,oy, nx,ny, &t))
					segment(map.off,
						Pt(ox, oy), Pt(nx, ny),
						z, DorS);
				ox = nx;
				oy = ny;
			}
			if(pointvis(ox, oy, &t))
				point(map.off,
					Pt(ox,oy),
					z, DorS);
		}
		for(r=b->points; r; r=r->link) {
			l = r->line;
			if(l == 0)
				continue;

			ox = l->lng*map.xscale + map.xoffset;
			oy = l->lat*map.yscale + map.yoffset;
			if(pointvis(ox, oy, &t))
				bitblt(map.off, Pt(ox-Msize/2, oy-Msize/2),
					map.pla, map.pla->r, DorS);
		}
	}

	gpsoff();

	nx = map.loc.lng*map.xscale + map.xoffset;
	ny = map.loc.lat*map.yscale + map.yoffset;
	bitblt(map.off, Pt(nx-Msize/2, ny-Msize/2),
		map.com, map.com->r, DorS);

	bflush();
	map.last = map.loc;
	cursorswitch(0);
}

void
regerror(char *e)
{

	USED(e);
}

void
hiseg(long ox, long oy, long nx, long ny, long z, Rec *r)
{
	long dx, dy;

	dx = ox-nx;
	if(dx < 0)
		dx = -dx;
	dy = oy-ny;
	if(dy < 0)
		dy = -dy;
	if(dx >= 500 || dy >= 500)
		return;

	if(!segvis(ox,oy, nx,ny, r))
		return;

	segment(map.off, Pt(ox,oy), Pt(nx,ny), z, DorS);
	segment(map.off, Pt(ox+1,oy), Pt(nx+1,ny), z, DorS);
	segment(map.off, Pt(ox-1,oy), Pt(nx-1,ny), z, DorS);

	segment(map.off, Pt(ox,oy), Pt(nx,ny), z, DorS);
	segment(map.off, Pt(ox,oy+1), Pt(nx,ny+1), z, DorS);
	segment(map.off, Pt(ox,oy-1), Pt(nx,ny-1), z, DorS);
}

void
hipoint(long ox, long oy, long z, Rec *r)
{
	if(!pointvis(ox, oy, r))
		return;

	point(map.off, Pt(ox,oy), z, DorS);

	point(map.off, Pt(ox+1,oy), z, DorS);
	point(map.off, Pt(ox-1,oy), z, DorS);

	point(map.off, Pt(ox,oy+1), z, DorS);
	point(map.off, Pt(ox,oy-1), z, DorS);
}

void
search(char *cmd)
{

	Box *b;
	Record *r;
	Line *l;
	Name *n;
	Sym *s;
	char *p;
	long nx, ny, ox, oy, z;
	int i;
	Reprog *re;

	cursorswitch(&thinking);

	p = strrchr(cmd, '/');
	if(p && p[1] == 0)
		*p = 0;
	re = regcomp(cmd);
	if(re == 0) {
		print("re error\n");
		return;
	}

	/*
	 * pass 1 -- match the re in the box hashtables
	 */
	for(b=map.boxlist; b; b=b->link)
		for(i=0; i<nelem(b->hash); i++)
			for(s=b->hash[i]; s; s=s->link)
				s->match = regexec(re, s->chars, 0, 0);
	free(re);

	/*
	 * pass 2 -- highlight all lines with matched names
	 */
	z = ~Z3;
	for(b=map.boxlist; b; b=b->link) {
		for(r=b->lines; r; r=r->link) {
			for(n=r->name; n; n=n->link)
				if(n->sym->match)
					break;
			if(n == 0)
				continue;
			l = r->line;
			if(l == 0)
				continue;

			ox = l->lng*map.xscale + map.xoffset;
			oy = l->lat*map.yscale + map.yoffset;
			for(l=l->link; l; l=l->link) {
				nx = l->lng*map.xscale + map.xoffset;
				ny = l->lat*map.yscale + map.yoffset;

				hiseg(ox,oy, nx,ny, z, &map.off->r);

				ox = nx;
				oy = ny;
			}
			hipoint(ox, oy, z, &map.off->r);
		}
	}

	bflush();
	cursorswitch(0);
}

void
loadbox(int boxx, int boxy, int scale)
{
	Box *b;

	for(b=map.boxlist; b; b=b->link)
		if(b->boxx == boxx)
			if(b->boxy == boxy)
				return;

	cursorswitch(&reading);
	b = newbox(boxx, boxy, scale);
	cursorswitch(0);

	if(b) {
		b->link = map.boxlist;
		map.boxlist = b;
		b->memchain = memchain();
	}
}

void
loadboxs(void)
{
	long maxbx, minbx, maxby, minby, perbox;
	int x, y, s;
	Box *b, **b1;

	s = 1;
	if(map.rscale >= Scale12) {
		s *= 5;
		if(map.rscale >= Scale23)
			s *= 5;
	}
	perbox = Perbox * s;

	minbx = (0 - map.xoffset) / map.xscale;
	maxbx = (map.width - map.xoffset) / map.xscale;
	minby = (map.height - map.yoffset) / map.yscale;
	maxby = (0 - map.yoffset) / map.yscale;

	minbx = minbx / perbox;
	maxbx = maxbx / perbox;
	minby = minby / perbox;
	maxby = maxby / perbox;

	/*
	 * free everything more than 1 box away
	 */
	for(b1=&map.boxlist; b=*b1;) {
		if(b->boxx >= minbx-1 && b->boxx <= minbx+1)
		if(b->boxy >= minby-1 && b->boxy <= minby+1)
		if(b->scale == s) {
			b1 = &b->link;
			continue;
		}

		*b1 = b->link;
		memfree(b->memchain);
		continue;
	}

	/*
	 * load all boxes in radius
	 */
	for(x=minbx; x<=maxbx; x++)
		for(y=minby; y<=maxby; y++)
			loadbox(x, y, s);
}

Sym*
strdict(Box *b, char *name)
{
	Sym *s;
	ulong h;
	int c;
	char *p, c0;

	h = 0;
	for(p = name; c = *p; p++) {
		h += h<<1;
		h += c;
	}
	h %= nelem(b->hash);
	c0 = name[0];
	for(s = b->hash[h]; s; s = s->link)
		if(s->chars[0] == c0)
			if(strcmp(s->chars, name) == 0)
				return s;

	s = ca(sizeof(*s));
	c = (p - name) + 1;
	s->chars = ca(c);
	memmove(s->chars, name, c);

	s->link = b->hash[h];
	b->hash[h] = s;

	return s;
}

Box*
newbox(int boxx, int boxy, int scale)
{
	Box *b;
	Record *r;
	Name *n, *n1;
	Line *l;
	char *p;
	long d;
	int latflag, latscale, z;
	char name[30];

	b = ca(sizeof(Box));
	b->boxx = boxx;
	b->boxy = boxy;
	b->scale = scale;

	if(scale == 1)
		sprint(name, "/lib/roads/%.4d/%.4d%.4d.h", boxy, boxy, boxx);
	else
	if(scale == 5)
		sprint(name, "/lib/roads/%.3d/%.3d%.3d.h", boxy, boxy, boxx);
	else
		sprint(name, "/lib/roads/%.2d/%.2d%.2d.h", boxy, boxy, boxx);

	if(Tinit(name))
		return b;

	r = 0;
	l = 0;
	latflag = 0;
	latscale = 0;

loop:
	p = Trdline();
	if(p == 0) {
		if(r) {
			if(r->line && !r->line->link) {
				r->link = b->points;
				b->points = r;
			} else {
				r->link = b->lines;
				b->lines = r;
			}
		}
		goto out;
	}
	switch(*p) {
	default:
		goto loop;

	case '1':
	case '4':
		if(r) {
			if(r->line && !r->line->link) {
				r->link = b->points;
				b->points = r;
			} else {
				r->link = b->lines;
				b->lines = r;
			}
			r = 0;
			l = 0;
		}
		goto loop;

	case 'c':
		if(r == 0)
			r = ca(sizeof(Record));
		r->grey = Z3;
		if(map.screend != 0) {
			z = p[1];
			if(z == 'B')
				r->grey = Z2;
			if(z == 'H')
				r->grey = Z1;
			if(z == 'X') {
				r->grey = Z1;
				if(p[3] == '3')
					r->grey = Z2;
			}
		}
		n = ca(sizeof(Name));
		n->sym = strdict(b, p+1);
		n1 = r->name;
		if(n1) {
			while(n1->link)
				n1 = n1->link;
			n1->link = n;
		} else
			r->name = n;
		goto loop;

	case 'n':
		if(r == 0)
			r = ca(sizeof(Record));
		n = ca(sizeof(Name));
		n->sym = strdict(b, p+1);
		n1 = r->name;
		if(n1) {
			while(n1->link)
				n1 = n1->link;
			n1->link = n;
		} else
			r->name = n;
		goto loop;

	case 'u':
		d = atol(p+1);
		latscale = 0;
		goto tu;

	case 't':
		d = atol(p+1)*5;
		latscale = 1;

	tu:
		if(l || !r)
			goto badll;
		l = ca(sizeof(Line));
		r->line = l;
		l->lat = d;
		latflag = 1;
		goto loop;

	case 'h':
		d = atol(p+1);
		latscale = 0;
		goto gh;

	case 'g':
		d = atol(p+1)*5;
		latscale = 1;

	gh:
		if(!latflag || !l)
			goto badll;
		l = r->line;
		l->lng = d;
		latflag = 0;
		goto loop;

	case 'd':
		d = atol(p+1);
		if(latscale)
			d = d*5;
		if(!l)
			goto badll;
		if(latflag) {
			d += l->lat;
			l->link = ca(sizeof(Line));
			l = l->link;
			l->lat = d;
			goto loop;
		}
		d += l->lng;
		l = l->link;
		if(!l)
			goto badll;
		l->lng = d;
		goto loop;

	badll:
		print("bad ll\n");
		goto loop;
	}

out:
	Tclose();

	/*
	 * calculate min/max x/y in each line
	 */
	for(r=b->lines; r; r=r->link) {
		l = r->line;
		if(l == 0)
			continue;
		r->linex1 = l->lng;
		r->linex2 = l->lng;
		r->liney1 = l->lat;
		r->liney2 = l->lat;
		for(l=l->link; l; l=l->link) {
			if(l->lng < r->linex1)
				r->linex1 = l->lng;
			if(l->lng > r->linex2)
				r->linex2 = l->lng;
			if(l->lat < r->liney1)
				r->liney1 = l->lat;
			if(l->lat > r->liney2)
				r->liney2 = l->lat;
		}
	}
	return b;
}

void
lookup1(Loc loc, int line)
{
	Box *b;
	Record *r, *bestr;
	Line *l;
	Name *n;
	long x, y, nx, ny, ox, oy, d, bestd;
	long bx1, bx2, by1, by2, perbox;

/*	cursorswitch(&thinking);	/**/

	bestd = 360*Perdegree;
	bestr = 0;
	x = loc.lng*map.xscale + map.xoffset;
	y = loc.lat*map.yscale + map.yoffset;
	bx1 = loc.lng - Close1;
	bx2 = loc.lng + Close1;
	by1 = loc.lat - Close1;
	by2 = loc.lat + Close1;
	for(b=map.boxlist; b; b=b->link) {
		perbox = Perbox*b->scale;
		d = b->boxx*perbox;
		if(d > bx2 || d+perbox < bx1)
			continue;
		d = b->boxy*perbox;
		if(d > by2 || d+perbox < by1)
			continue;

		for(r=b->lines; r; r=r->link) {
			/*
			 * check bounding box
			 */
			if(r->linex1 > bx2 || r->linex2 < bx1)
				continue;
			if(r->liney1 > by2 || r->liney2 < by1)
				continue;

			l = r->line;
			if(l == 0)
				continue;

			ox = l->lng*map.xscale + map.xoffset;
			oy = l->lat*map.yscale + map.yoffset;

			for(l=l->link; l; l=l->link) {
				nx = l->lng*map.xscale + map.xoffset;
				ny = l->lat*map.yscale + map.yoffset;

				if(segvis(ox,oy, nx,ny, &map.off->r)) {
					d = dtoline(ox, oy, nx, ny, x, y);
					if(bestr == 0 || d < bestd) {
						bestd = d;
						bestr = r;
					}
				}

				ox = nx;
				oy = ny;
			}
		}
	}

	map.p1.line[line][0] = 0;
	if(bestr)
	for(n=bestr->name; n; n=n->link) {
		if(strlen(map.p1.line[line])+strlen(n->sym->chars)
			>= sizeof(map.p1.line[line])-4)
				break;
		if(*map.p1.line[line] != 0)
			strcat(map.p1.line[line], "; ");
		strcat(map.p1.line[line], n->sym->chars);
	}

/*	cursorswitch(0);	/**/
}

void
lookup2(Loc loc, int line)
{
	Box *b;
	Record *r, *bestr;
	Line *l;
	Name *n;
	long x, y, ox, oy, d, bestd;
	double f;

/*	cursorswitch(&thinking);	/**/

	bestd = 360*Perdegree;
	bestr = 0;
	x = loc.lng*map.xscale + map.xoffset;
	y = loc.lat*map.yscale + map.yoffset;
	for(b=map.boxlist; b; b=b->link) {
		for(r=b->points; r; r=r->link) {
			l = r->line;
			if(l == 0)
				continue;

			ox = l->lng*map.xscale + map.xoffset;
			oy = l->lat*map.yscale + map.yoffset;
			d = dtoline(ox, oy, ox, oy, x, y);
			if(bestr == 0 || d < bestd) {
				bestd = d;
				bestr = r;
			}
		}
	}


	map.p1.line[line][0] = 0;
	if(bestr)
	for(n=bestr->name; n; n=n->link) {
		if(strlen(map.p1.line[line])+strlen(n->sym->chars)
			>= sizeof(map.p1.line[line])-4)
					break;
		if(*map.p1.line[line] != 0)
			strcat(map.p1.line[line], "; ");
		strcat(map.p1.line[line], n->sym->chars);
	}
	if(strlen(map.p1.line[line])+20 < sizeof(map.p1.line[line])) {
		if(*map.p1.line[line] != 0)
			strcat(map.p1.line[line], "; ");
		f = (double)loc.lat/Perdegree;
		if(f < 90)
			sprint(strchr(map.p1.line[line], 0), "%.4fN", f);
		else
			sprint(strchr(map.p1.line[line], 0), "%.4fS", 360-f);
		
		f = (double)loc.lng/Perdegree;
		if(f < 180)
			sprint(strchr(map.p1.line[line], 0), " %.4fE", f);
		else
			sprint(strchr(map.p1.line[line], 0), " %.4fW", 360-f);

	}

/*	cursorswitch(0);	/**/
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
drawmap(Loc loc)
{
	int x, y, redraw, dolook;

	redraw = 0;
	dolook = 0;

	if(map.here.lat != loc.lat || map.here.lng != loc.lng)
		dolook = 1;

	map.here = loc;
	if(map.loc.lat == 0 && map.loc.lng == 0) {
		map.loc = loc;

		/*
		 * 4 linear parameters for map coordinates
		 */
	rescale:
		map.xscale = 1.0e-3 / map.logsca[map.rscale];
		map.yscale = -1.0e-3 / map.logsca[map.rscale] /
			cos(map.loc.lat/(57.29578*Perdegree));
		map.xoffset = map.width/2. - map.loc.lng*map.xscale;
		map.yoffset = map.height/2. - map.loc.lat*map.yscale;
		redraw = 1;
		dolook = 1;
	}

	x = (map.here.lng - map.loc.lng)*map.xscale;
	y = (map.here.lat - map.loc.lat)*map.yscale;
	x += (map.width - (map.screenr.max.x - map.screenr.min.x))/2;
	y += (map.height - (map.screenr.max.y - map.screenr.min.y))/2;

	if(x < 0 || y < 0 ||
	   x + (map.screenr.max.x - map.screenr.min.x) >= map.width ||
	   y + (map.screenr.max.y - map.screenr.min.y) >= map.height) {
		map.loc = adjloc();
		goto rescale;
	}

	if(redraw) {
		loadboxs();
		drawoff();
	}
	if(dolook) {
		lookup1(loc, 0);
		lookup2(loc, 1);
	}

	bitblt(&screen,
		map.screenr.min,
		map.off,
		Rect(	x,
			y,
			x + (map.screenr.max.x - map.screenr.min.x),
			y + (map.screenr.max.y - map.screenr.min.y)),
		S);

	bitblt(&screen,
		Pt(	(map.screenr.min.x + map.screenr.max.x)/2 - Msize/2,
			(map.screenr.min.y + map.screenr.max.y)/2 - Msize/2),
		map.poi, map.poi->r, DorS);

	drawpage(&map.p1);
	bflush();
}
