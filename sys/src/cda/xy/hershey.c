#include <u.h>
#include <libc.h>
#include <libg.h>
#include <bio.h>
#include "dat.h"
#include "fns.h"

static FPoint	xform(FPoint, FPoint, int, FPoint);

static char	ocfile[] = "/lib/font/hershey/hersh.oc";

static Biobuf *	hbp;

Hfont *
hersheyfont(char *name)
{
	Biobuf *bp;
	char buf[128], *l;
	int i;
	Hfont *hf;

	if(hbp == 0){
		hbp = Bopen(ocfile, OREAD);
		if(hbp == 0){
			perror(ocfile);
			return 0;
		}
	}
	sprint(buf, "/lib/font/hershey/%s.addr", name);
	bp = Bopen(buf, OREAD);
	if(bp == 0){
		perror(buf);
		return 0;
	}
	hf = malloc(sizeof(Hfont));
	memset(hf->h, 0, sizeof hf->h);
	for(i=0; i<96; i++){
		l = Brdline(bp, '\n');
		hf->addr[i] = strtol(l, 0, 10);
	}
	Bclose(bp);
	return hf;
}

Hershey *
hersheychar(Hfont *hf, int c)
{
	Hershey *h;
	int gnum, size;
	char buf[8];

	c -= 32;
	if(c < 0 || c >= 96)
		return 0;
	if(h = hf->h[c])	/* assign = */
		return h;
	Bseek(hbp, hf->addr[c], 0);
	Bread(hbp, buf, 5);
	buf[5] = 0;
	gnum = strtol(buf, 0, 10);
	Bread(hbp, buf, 3);
	buf[3] = 0;
	size = 2*strtol(buf, 0, 10);
	h = malloc(sizeof(Hershey)+size-1);
	h->gnum = gnum;
	Bread(hbp, h->stroke, size);
	h->stroke[size] = 0;
	hf->h[c] = h;
	return h;
}

int 
hersheywid(Hfont *hf, int c)
{
	Hershey *h = hersheychar(hf, c);

	return h ? h->stroke[1] - h->stroke[0] : -1;
}

#define	FPt(x, y)	(FPoint){(float)(x), (float)(y)}

static FPoint
xform(FPoint p, FPoint c, int angle, FPoint m)
{
	static int cangle;
	static double ccos, csin;
	double theta;

	switch(angle){
	case 0:
		return FPt(c.x+m.x*p.x, c.y+m.y*p.y);
	case 90:
		return FPt(c.x-m.y*p.y, c.y+m.x*p.x);
	case 180:
		return FPt(c.x-m.x*p.x, c.y-m.y*p.y);
	case -90:
	case 270:
		return FPt(c.x+m.y*p.y, c.y-m.x*p.x);
	}
	if(angle != cangle){
		cangle = angle;
		theta = PI*angle/180.0;
		ccos = cos(theta);
		csin = sin(theta);
	}
	return FPt(c.x + m.x*p.x*ccos - m.y*p.y*csin,
		  c.y + m.x*p.x*csin + m.y*p.y*ccos);
}

Hfont *	hfont;

void
gerbertext(Text *tp)
{
	Hfont *hf; Hershey *h;
	FPoint p, q;
	FPoint mag;
	int i, c; char *s;
	int linewid = 10;
	float ht, wd, sp;

	if(!hfont)
		hfont = hersheyfont("romanp");

	p = FPt(tp->start.x, tp->start.y);
	ht = tp->size - linewid;
	sp = 0.8*tp->size;
	wd = sp - linewid;
	mag.y = ht/9.0;
	s = tp->text->name;
	i = 0;
	gerberaper(20 + linewid);
	while(c = *s++){	/* assign = */
		q = xform(FPt(i++, 0), p, tp->angle, FPt(sp, 1.0));
		h = hersheychar(hfont, c);
		if(h == 0)
			continue;
		mag.x = wd/(h->stroke[1] - h->stroke[0]);
		hersheydraw(h, q, tp->angle, mag);
	}
}

void
hersheydraw(Hershey *h, FPoint c, int angle, FPoint mag)
{
	uchar *p;
	FPoint p0;
	int pen;

	pen = '2';
	for(p = &h->stroke[2]; *p; p+=2){
		if(*p == ' '){
			pen = '2';
			continue;
		}
		p0 = xform(FPt(p[0]-L'R', L'R'-p[1]-0.5), c, angle, mag);
		Bprint(&out, "X%GY%GD0%c*\n",
			(int)(p0.x+0.5), (int)(p0.y+0.5), pen);
		pen = '1';
	}
}
