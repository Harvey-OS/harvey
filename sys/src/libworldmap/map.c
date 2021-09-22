#include	<u.h>
#include	<libc.h>
#include	<thread.h>
#include	<ctype.h>
#include	<bio.h>
#include	<draw.h>
#include	<keyboard.h>
#include	<mouse.h>
#include	<worldmap.h>
#include	"mapimpl.h"

char		*mapbase = "/lib/worldmap";
long		debug;

uchar minimap[180*360];

void
freepoly(Poly *p)
{
	if (p == nil)
		return;
	if (p->p)
		free(p->p);
	free(p);
}

void
freepolys(Polys *pol)
{
	int i;

	if (pol == nil)
		return;
	for (i = 0; i < pol->npol; i++)
		freepoly(pol->pol[i]);
}

void
freetile(void *arg)
{
	int i;
	Maptile *tile;

	if (arg == nil)
		return;
	tile = arg;
	freeedge(tile->edgepts);

	freepolys(&tile->cp);
	for (i = 0; i < 4; i++)
		freepolys(&tile->features[i]);

	free(tile);
}

void*
readtile(int lat, int lon, int res)
{
	Maptile *m;
	char *buf, *p;
	Biobuf *bin;
	Poly *pol;
	char fname[128];
	int tp;
	Polys *pls;

	if (lat < -90 || lat >= 90)
		return nil;
	while (lon <    0) lon += 360;
	while (lon >= 360) lon -= 360;

	switch (res) {
		default:
			return nil;
		case 1:
			break;
		case 2:
		case 5:
		case 10:
		case 20:
			lat = lat - ((lat + 90) % res);
			lon = lon - (lon % res);
	}
	m = mallocz(sizeof(Maptile), 1);

	m->lat = lat;
	m->lon = lon;
	m->res = res;
	m->fr = Rect(lon*1000000, lat*1000000,
			  (lon+res)*1000000, (lat+res)*1000000);

	switch(minimap[360*(89-lat)+lon]) {
	case 123:
		m->background = Land;
		break;
	case 188:
		m->background = Sea;
		break;
	default:
		m->background = Unknown;
		break;
	}

	if (debug & 0x2)
		fprint(2, "lat %d, lon %d, res %d, background %s\n",
			lat, lon, res,
			m->background == Land?"land":m->background == Sea?"Sea":"Unknown");

	sprint(fname, "%s/%dx%d/%d/%d:%d:%d",
		mapbase, m->res, m->res, m->lon, m->lon, m->lat, m->res);
	if ((bin = Bopen(fname, OREAD)) == nil) {
		if (debug)
			fprint(2, "readcoast: %s: %r\n", fname);
		return m;
	}
	if (debug & 0x2)
		fprint(2, "processing %s\n", fname);

	pol = nil;
	tp = -1;
	while ((buf = Brdline(bin, '\n')) && buf[0] != '\0') {
		static long la, ln;
		Point pt;

		if (buf[0] == '#')
			continue;
		if (isdigit(buf[0]) || buf[0] == '-') {
			p = buf;
			if (pol == nil || pol->np == 0) {
				ln = strtol(p, &p, 10);
				la = strtol(p, &p, 10);
			} else {
				ln += strtol(p, &p, 10);
				la += strtol(p, &p, 10);
			}
			pt = Pt(ln, la);
			if (pol == nil)
				pol = mallocz(sizeof(Poly), 1);
			if (pol->np > 0 && eqpt(pol->p[pol->np-1], pt))
				continue;
			pol->p = realloc(pol->p, (pol->np + 1)*sizeof(Point));
			pol->p[pol->np++] = pt;
			continue;
		}
		if (buf[0] == '>') {
			if (pol && pol->np > 0) {
				switch (tp) {
				case Sea:
					finishpoly(m, pol);
					break;
				case Border:
				case Stateline:
				case River:
				case Riverlet:
					pls = &m->features[tp];
					pls->pol = realloc(pls->pol, (pls->npol+1)*sizeof(Poly*));
					pls->pol[pls->npol] = pol;
					pls->npol++;
					break;
				}
			}
			pol = nil;
			if (strncmp(buf+1, "Coast", 5) == 0)
				tp = Sea;
			else if (strncmp(buf+1, "Riverlet", 8) == 0)
				tp = Riverlet;
			else if (strncmp(buf+1, "River", 5) == 0)
				tp = River;
			else if (strncmp(buf+1, "Border", 6) == 0)
				tp = Border;
			else if (strncmp(buf+1, "Stateline", 9) == 0)
				tp = Stateline;
		}
	}
	switch (tp) {
	case Sea:
		finishpoly(m, pol);
		break;
	case Border:
	case Stateline:
	case River:
	case Riverlet:
		pls = &m->features[tp];
		pls->pol = realloc(pls->pol, (pls->npol+1)*sizeof(Poly*));
		pls->pol[pls->npol] = pol;
		pls->npol++;
		break;
	}
	Bterm(bin);

	return m;
}

void
drawtile(void *x, int flags, void *arg)
{
	Maptile *m;

	m = x;
	if (flags & ((1<<Sea)|(1<<Land))) drawcoast(m, flags, arg);
	if (flags & (1<<Coastline)) drawcoastline(m, arg);
	if (flags & (1<<Stateline)) drawriver(m, Stateline, arg);
	if (flags & (1<<Border)) drawriver(m, Border, arg);
	if (flags & (1<<Riverlet)) drawriver(m, Riverlet, arg);
	if (flags & (1<<River)) drawriver(m, River, arg);
}

void
mapinit(void)
{
	char str[128];
	int fd;

	sprint(str, "%s/world.map", mapbase);
	if ((fd = open(str, OREAD)) < 0)
		sysfatal("%s: %r", str);
	if (read(fd, minimap, 360*180) != 360*180)
		sysfatal("read %s: %r", str);
	close(fd);
}
