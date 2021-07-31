#include <u.h>
#include <libc.h>
#include	<cda/fizz.h>
void f_minor(char *, ...);
void f_major(char *, ...);

#define		NBUF		50000

static Keymap keys[] = {
	"name", NULLFN, 1,
	"br", NULLFN, 2,
	"pins", NULLFN, 3,
	"drills", NULLFN, 4,
	"wires", NULLFN, 5,
	"plane", NULLFN, 6,
	"xymask", NULLFN, 7,
	0
};

void
f_package(char *s)
{
	int nm, i;
	Package *p;
	int lo, hi;
	char *os;
	char buf[NBUF], *nxt;
	Plane c[100], k[100];
	int nc = 0, nk = 0;

	BLANK(s);
	if(*s != '{'){
		f_minor("package needs a '{'\n");
		return;
	}
	p = NEW(Package);
	while(s = f_inline()){
		BLANK(s);
		if(*s == '}') break;
		switch(f_keymap(keys, &s))
		{
		case 1:
			p->name = f_strdup(s);
			break;
		case 2:
			NUM(s, p->r.min.x)
			NUM(s, p->r.min.y)
			NUM(s, p->r.max.x)
			NUM(s, p->r.max.y)
			if(*s) EOLN(s)
			break;
		case 3:
			NUM(s, p->pmin)
			NUM(s, p->pmax)
			p->npins = p->pmax-p->pmin+1;
			p->pins = f_pins(s, p->pmin, p->npins);
			break;
		case 4:
			NUM(s, lo)
			NUM(s, hi)
			p->ndrills = hi-lo+1;
			p->drills = f_pins(s, lo, hi);
			break;
		case 5:
			BLANK(s)
			if(isdigit(*s)) {
				k[nk].layer = *s-'0';
				s++;
				BLANK(s);
			}
			else
				k[nk].layer = 3;
			if(*s == '+')
				k[nk].sense = 1;
			else if(*s == '-')
				k[nk].sense = -1;
			else
				f_minor("expected a sense [+-], got '%c'", *s);
			s++;
			BLANK(s);
			NUM(s, k[nk].r.min.x)
			NUM(s, k[nk].r.min.y)
			NUM(s, k[nk].r.max.x)
			NUM(s, k[nk].r.max.y)
			k[nk].sig = 0;
			nk++;
			break;
		case 6:
			NUM(s, i)
			if((i < 0) || (i >= MAXLAYER))
				f_minor("bad layer number %d", i);
			c[nc].layer = LAYER(i);
			BLANK(s);
			if(*s == '+')
				c[nc].sense = 1;
			else if(*s == '-')
				c[nc].sense = -1;
			else
				f_minor("expected a sense [+-], got '%c'", *s);
			s++;
			BLANK(s);
			os = s;
			NAME(s);
			c[nc].sig = f_strdup(os);
			NUM(s, c[nc].r.min.x)
			NUM(s, c[nc].r.min.y)
			NUM(s, c[nc].r.max.x)
			NUM(s, c[nc].r.max.y)
			c[nc].sense = -1;
			c[nc].sig = 0;
			nc++;
			break;
		case 7:
			BLANK(s)
			os = s;
			NAME(s)
			p->xyid = f_strdup(os);
			BLANK(s)
			if(*s != '{')
				break;
			nxt = buf;
			while(s = f_tabline()){
				if(*s == '}') break;
				lo = strlen(s);
				if(nxt+lo+1 >= buf+NBUF){
					f_major("XY mask defn for %s too big (>= %d)\n", p->xyid, NBUF);
				}
				memcpy(nxt, s, lo);
				nxt += lo;
				*nxt++ = '\n';
			}
			*nxt++ = 0;
			lo = nxt-buf;
			p->xydef = lmalloc((long) lo);
			memcpy(p->xydef, buf, lo);
			break;
		default:
			f_minor("bad field in package '%s'", s);
			break;
		}
	}
	if(p->ncutouts = nc){
		p->cutouts = (Plane *) lmalloc((long) (nc = nc*sizeof(Plane)));
		memcpy((char *)p->cutouts, (char *)c, nc);
	}
	if(p->nkeepouts = nk){
		p->keepouts = (Plane *) lmalloc((long) (nk = nk*sizeof(Plane)));
		memcpy((char *)p->keepouts, (char *)k, nk);
	}
	if(symlook(p->name, S_PACKAGE, (void *)0))
		f_major("redefinition of package %s", p->name);
	else
		(void)symlook(p->name, S_PACKAGE, (void *)p);
}

void
chkpkg(Package *p)
{
	int i;
	Pin pin;

	if((p->r.max.x == 0) || (p->r.max.y == 0))
		f_major("package %s: bad bounding rect lens %d,%d",
			p->name, p->r.max.x, p->r.max.y);
	if(p->npins){
		if(p->pins == 0)
			f_major("package %s: no pins defined", p->name);
		for(i = 0; i < p->npins; i++){
			pin = p->pins[i];
			if((pin.p.x < p->r.min.x) || (pin.p.x >= p->r.max.x)
				|| (pin.p.y < p->r.min.y) || (pin.p.y >= p->r.max.y))
				f_major("package %s: pin %d[%p] outside br", p->name, i+p->pmin, pin.p);
		}
		for(i = 1; i < p->npins; i++)
			fg_psub(&p->pins[i].p, p->pins[0].p);
		for(i = 0; i < p->ndrills; i++)
			fg_psub(&p->drills[i].p, p->pins[0].p);
		for(i = 0; i < p->ncutouts; i++) {
			fg_psub(&p->cutouts[i].r.min, p->pins[0].p);
			fg_psub(&p->cutouts[i].r.max, p->pins[0].p);
		}
		for(i = 0; i < p->nkeepouts; i++) {
			fg_psub(&p->keepouts[i].r.min, p->pins[0].p);
			fg_psub(&p->keepouts[i].r.max, p->pins[0].p);
		}
		fg_psub(&p->r.min, p->pins[0].p);
		fg_psub(&p->r.max, p->pins[0].p);
		p->xyoff.x = p->xyoff.y = 0;
		fg_psub(&p->xyoff, p->pins[0].p);
		fg_psub(&p->pins[0].p, p->pins[0].p);
	}
}
