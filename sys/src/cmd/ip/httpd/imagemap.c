#include <u.h>
#include <libc.h>
#include <bio.h>
#include "httpd.h"
#include "httpsrv.h"

typedef struct Point	Point;
typedef struct OkPoint	OkPoint;
typedef struct Strings	Strings;

struct Point 
{
	int	x;
	int	y;
};

struct OkPoint 
{
	Point	p;
	int	ok;
};

struct Strings
{
	char	*s1;
	char	*s2;
};

static	char *me;

int		polytest(int, Point, Point, Point);
Strings		getfield(char*);
OkPoint		pt(char*);
char*		translate(HConnect*, char*, char*);
Point		sub(Point, Point);
float		dist(Point, Point);

void
main(int argc, char **argv)
{
	HConnect *c;
	Hio *hout;
	char *dest;

	me = "imagemap";
	c = init(argc, argv);
	hout = &c->hout;
	if(hparseheaders(c, HSTIMEOUT) < 0)
		exits("failed");
	anonymous(c);

	if(strcmp(c->req.meth, "GET") != 0 && strcmp(c->req.meth, "HEAD") != 0){
		hunallowed(c, "GET, HEAD");
		exits("unallowed");
	}
	if(c->head.expectother || c->head.expectcont){
		hfail(c, HExpectFail, nil);
		exits("failed");
	}
	dest = translate(c, c->req.uri, c->req.search);

	if(dest == nil){
		if(c->req.vermaj){
			hokheaders(c);
			hprint(hout, "Content-type: text/html\r\n");
			hprint(hout, "\r\n");
		}
		hprint(hout, "<head><title>Nothing Found</title></head><body>\n");
		hprint(hout, "Nothing satisfying your search request could be found.\n</body>\n");
		hflush(hout);
		writelog(c, "Reply: 200 imagemap %ld %ld\n", hout->seek, hout->seek);
		exits(nil);
	}

	if(http11(c) && strcmp(c->req.meth, "POST") == 0)
		hredirected(c, "303 See Other", dest);
	else
		hredirected(c, "302 Found", dest);
	exits(nil);
}

char*
translate(HConnect *c, char *uri, char *search)
{
	Biobuf *b;
	Strings ss;
	OkPoint okp;
	Point p, cen, q, start;
	float close, d;
	char *line, *to, *def, *s, *dst;
	int n, inside, r, ncsa;

	if(search == nil){
		hfail(c, HNoData, me);
		exits("failed");
	}
	okp = pt(search);
	if(!okp.ok){
		hfail(c, HBadSearch, me);
		exits("failed");
	}
	p = okp.p;

	b = Bopen(uri, OREAD);
	if(b == nil){
		hfail(c, HNotFound, uri);
		exits("failed");
	}

	to = nil;
	def = nil;
	dst = nil;
	close = 0.;
	ncsa = 1;
	while(line = Brdline(b, '\n')){
		line[Blinelen(b)-1] = 0;

		ss = getfield(line);
		s = ss.s1;
		line = ss.s2;
		if(ncsa){
			ss = getfield(line);
			dst = ss.s1;
			line = ss.s2;
		}
		if(strcmp(s, "#cern") == 0){
			ncsa = 0;
			continue;
		}
		if(strcmp(s, "rect") == 0){
			ss = getfield(line);
			s = ss.s1;
			line = ss.s2;
			okp = pt(s);
			q = okp.p;
			if(!okp.ok || q.x > p.x || q.y > p.y)
				continue;
			ss = getfield(line);
			s = ss.s1;
			line = ss.s2;
			okp = pt(s);
			q = okp.p;
			if(!okp.ok || q.x < p.x || q.y < p.y)
				continue;
			if(!ncsa){
				ss = getfield(line);
				dst = ss.s1;
			}
			return dst;
		}else if(strcmp(s, "circle") == 0){
			ss = getfield(line);
			s = ss.s1;
			line = ss.s2;
			okp = pt(s);
			cen = okp.p;
			if(!okp.ok)
				continue;
			ss = getfield(line);
			s = ss.s1;
			line = ss.s2;
			if(ncsa){
				okp = pt(s);
				if(!okp.ok)
					continue;
				if(dist(okp.p, cen) >= dist(p, cen))
					return dst;
			}else{
				r = strtol(s, nil, 10);
				ss = getfield(line);
				dst = ss.s1;
				d = (float)r * r;
				if(d >= dist(p, cen))
					return dst;
			}
		}else if(strcmp(s, "poly") == 0){
			ss = getfield(line);
			s = ss.s1;
			line = ss.s2;
			okp = pt(s);
			start = okp.p;
			if(!okp.ok)
				continue;
			inside = 0;
			cen = start;
			for(n = 1; ; n++){
				ss = getfield(line);
				s = ss.s1;
				line = ss.s2;
				okp = pt(s);
				q = okp.p;
				if(!okp.ok)
					break;
				inside = polytest(inside, p, cen, q);
				cen = q;
			}
			inside = polytest(inside, p, cen, start);
			if(!ncsa)
				dst = s;
			if(n >= 3 && inside)
				return dst;
		}else if(strcmp(s, "point") == 0){
			ss = getfield(line);
			s = ss.s1;
			line = ss.s2;
			okp = pt(s);
			q = okp.p;
			if(!okp.ok)
				continue;
			d = dist(p, q);
			if(!ncsa){
				ss = getfield(line);
				dst = ss.s1;
			}
			if(d == 0.)
				return dst;
			if(close == 0. || d < close){
				close = d;
				to = dst;
			}
		}else if(strcmp(s, "default") == 0){
			if(!ncsa){
				ss = getfield(line);
				dst = ss.s1;
			}
			def = dst;
		}
	}
	if(to == nil)
		to = def;
	return to;
}

int
polytest(int inside, Point p, Point b, Point a)
{
	Point pa, ba;

	if(b.y>a.y){
		pa=sub(p, a);
		ba=sub(b, a);
	}else{
		pa=sub(p, b);
		ba=sub(a, b);
	}
	if(0<=pa.y && pa.y<ba.y && pa.y*ba.x<=pa.x*ba.y)
		inside = !inside;
	return inside;
}

Point
sub(Point p, Point q)
{
	p.x -= q.x;
	p.y -= q.y;
	return p;
}

float
dist(Point p, Point q)
{
	p.x -= q.x;
	p.y -= q.y;
	return (float)p.x * p.x + (float)p.y * p.y;
}

OkPoint
pt(char *s)
{
	OkPoint okp;
	Point p;
	char *t, *e;

	if(*s == '(')
		s++;
	t = strchr(s, ')');
	if(t != nil)
		*t = 0;
	p.x = 0;
	p.y = 0;
	t = strchr(s, ',');
	if(t == nil){
		okp.p = p;
		okp.ok = 0;
		return okp;
	}
	e = nil;
	p.x = strtol(s, &e, 10);
	if(e != t){
		okp.p = p;
		okp.ok = 0;
		return okp;
	}
	p.y = strtol(t+1, &e, 10);
	if(e == nil || *e != 0){
		okp.p = p;
		okp.ok = 0;
		return okp;
	}
	okp.p = p;
	okp.ok = 1;
	return okp;
}

Strings
getfield(char *s)
{
	Strings ss;
	char *f;

	while(*s == '\t' || *s == ' ')
		s++;
	f = s;
	while(*s && *s != '\t' && *s != ' ')
		s++;
	if(*s)
		*s++ = 0;
	ss.s1 = f;
	ss.s2 = s;
	return ss;
}
