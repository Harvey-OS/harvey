#include	<all.h>

void
drawlist(double minlng, double maxlng, double minlat, double maxlat)
{
	uchar *dclass;
	Rec *f;
	Loc *l;
	int x, y, z, i, co, dx, dy, any;
	double d;
	int b[5];
	Waitmsg wmsg;
	Biobuf bio;

	pipe(b);
	if(fork()) {
		/* parent -- writer */
		close(b[0]);
		Binit(&bio, b[1], OWRITE);
	} else {
		/* child -- reader */
		close(b[1]);
		Binit(&bio, b[0], OREAD);
		for(;;) {
			i = Bread(&bio, b, sizeof(b));
			if(i != sizeof(b))
				break;
			dx = b[0]-b[2];
			if(dx < 0)
				dx = -dx;
			dy = b[1]-b[3];
			if(dy < 0)
				dy = -dy;
			if(dx <= 1 && dy <= 1)
				point(map.off, Pt(b[0],b[1]), b[4], DorS);
			else
				segment(map.off, Pt(b[0],b[1]), Pt(b[2],b[3]), b[4], DorS);
		}
		bflush();
		_exits(0);
	}

	x = 0;
	y = 0;
	dclass = dispclass[rscale];
	for(f=reclist; f; f=f->link) {
		if(dclass[f->type] == 0)
			continue;
		z = Z3;
		if(lightclass[f->type] != 0)
			z = Z1;		/* water feature in light grey */
		if(darkclass[f->type] != 0)
			z = Z2;		/* rail feature in dark grey */
		co = 0;
		l = f->loc;
		any = 0;
		for(i=f->nloc; i>0; i--,l++) {
			d = l->lat;
			if(d < minlat || d > maxlat) {
				any = 1;
				co = 0;
				continue;
			}
			d = l->lng;
			if(d < minlng || d > maxlng) {
				any = 1;
				co = 0;
				continue;
			}
			mapcoord(l);
			if(map.x == 0) {
				any = 1;
				co = 0;
				continue;
			}
			if(co == 0) {
				co = 1;
				x = map.x;
				y = map.y;
				continue;
			}
			if(x != map.x || y != map.y) {
				b[0] = x;
				b[1] = y;
				b[2] = map.x;
				b[3] = map.y;
				b[4] = z;
				Bwrite(&bio, b, sizeof(b));
				any = 1;
			}
			x = map.x;
			y = map.y;
		}
		if(any == 0) {
			b[0] = map.x;
			b[1] = map.y;
			b[2] = map.x;
			b[3] = map.y;
			b[4] = z;
			Bwrite(&bio, b, sizeof(b));
		}
	}
	Bclose(&bio);
	close(bio.fid);
	wait(&wmsg);
}

void
regerror(char *e)
{

	USED(e);
}

void
search(char *cmd)
{
	Rec *f;
	Loc *l;
	int x, y, i, co;
	Reprog *re;

	re = regcomp(cmd);
	if(re == 0)
		return;

	x = 0;
	y = 0;
	for(f=reclist; f; f=f->link) {
		if(f->nname != 1) {
			co = 0;
			for(i=0; i<f->nname; i++) {
				co = regexec(re, f->names[i], 0, 0);
				if(co)
					break;
			}
		} else
			co = regexec(re, f->name, 0, 0);
		if(co == 0)
			continue;

		co = 0;
		l = f->loc;
		for(i=f->nloc; i>0; i--,l++) {
			mapcoord(l);
			if(map.x == 0) {
				co = 0;
				continue;
			}
			if(co == 0) {
				co = 1;
				x = map.x;
				y = map.y;
				continue;
			}
			if(x != map.x || y != map.y) {
				segment(map.off, Pt(x+1,y), Pt(map.x+1,map.y), Z3, DorS);
				segment(map.off, Pt(x-1,y), Pt(map.x-1,map.y), Z3, DorS);
				segment(map.off, Pt(x,y+1), Pt(map.x,map.y+1), Z3, DorS);
				segment(map.off, Pt(x,y-1), Pt(map.x,map.y-1), Z3, DorS);
			} else
				point(map.off, Pt(x,y), Z3, DorS);
			x = map.x;
			y = map.y;
		}
	}
	bflush();
	free(re);
}

void
outrec(void)
{
	Rec *r;
	Loc *l;
	int n, i;

	if(rec.nlat >= nelem(rec.lat))
		goto out;
	if(rec.nname < 0 || rec.nname >= nelem(rec.name))
		goto out;
	n = rec.nlat/2;
	if(n+n != rec.nlat)
		goto out;
	r = malloc(sizeof(*r));
	if(r == 0){
		fprint(2, "malloc failed: %r\n");
		exits(0);
	}
	r->type = rec.type;

	l = malloc(n*sizeof(*l));
	if(l == 0){
		fprint(2, "malloc failed: %r\n");
		exits(0);
	}
	r->loc = l;
	r->nloc = n;
	for(i=0; i<n; i++) {
		l->lat = norm(rec.lat[i+0]/(DEGREE(5e6)), TWOPI);
		l->lng = norm(rec.lat[i+n]/(DEGREE(5e6)), TWOPI);
		l->sinlat = sin(l->lat);
		l->coslat = cos(l->lat);
		l++;
	}

	n = rec.nname;
	if(n > 1) {
		r->names = malloc(n*sizeof(r->name));
		for(i=0; i<n; i++)
			r->names[i] = rec.name[i];
	} else
		r->name = rec.name[0];
	r->nname = n;

	r->link = reclist;
	reclist = r;

out:;
}

char*
strdict(char *name)
{
	Sym *s;
	long h;
	char *p, c0;

	h = 0;
	for(p=name; *p;) {
		h *= 3;
		h += *p++;
	}
	if(h < 0)
		h = ~h;
	h %= nelem(hash);
	c0 = name[0];
	for(s = hash[h]; s; s = s->link) {
		if(s->name[0] != c0)
			continue;
		if(strcmp(s->name, name) == 0)
			return s->name;
	}
	s = malloc(sizeof(*s));
	s->name = malloc((p-name)+1);
	strcpy(s->name, name);

	s->link = hash[h];
	hash[h] = s;

	return s->name;
}

int
class(char *cfcc)
{
	int n;

	n = cfcc[1]*10 + cfcc[2] - ('0'*10 + '0');
	if(n < 0 || n >= 100)
		return 0;
	switch(cfcc[0]) {
	default:
		return 0;
	case 'A':
		return classa[n];
	case 'B':
		return classb[n];
	case 'C':
		return classc[n];
	case 'D':
		return classd[n];
	case 'E':
		return classe[n];
	case 'F':
		return classf[n];
	case 'H':
		return classh[n];
	case 'X':
		return classx[n];
	}
}

void
convcl(char *p, int ty)
{
	int i, j, k;

	p[3] = 0;
	for(i='A'; i<='X'; i++) {
		p[0] = i;
		for(j='0'; j<='9'; j++) {
			p[1] = j;
			for(k='0'; k<='9'; k++) {
				p[2] = k;
				if(class(p) == ty)
					return;
			}
		}
	}
}

void
readdict(char *name, Loc *loc, int init)
{
	int i;
	char *p, *side;
	double minlng, maxlng, minlat, maxlat, d;
	uchar *dclass;
	Loc *l;
	Rec *r, *s;
	Sym *t, *u;

	if(init) {
		for(r = reclist; r; r = s) {
			s = r->link;
			if(r->loc)
				free(r->loc);
			if(r->nname > 1)
				free(r->names);
			free(r);
		}
		reclist = 0;
		for(i=0; i<nelem(hash); i++)
			for(t = hash[i]; t; t = u) {
				u = t->link;
				if(t->name)
					free(t->name);
				free(t);
			}
		memset(hash, 0, sizeof(hash));
	}

	if(Tinit(name)) {
		fprint(2, "open: %s: %r\n", name);
		return;
	}

loop:
	p = Trdline();

loop1:
	if(p == 0)
		goto out;
	if(*p != '1') {
		p = Trdline();
		goto loop1;
	}

	p = Trdline();
	if(p == 0)
		goto out;
	if(*p != 'c')
		goto loop;
	i = class(p+1);
	if(saveclass[i] == 0)
		goto loop;
	rec.type = i;
	rec.nname = 0;
	rec.nlat = 0;
	side = 0;

loop2:
	p = Trdline();
	if(p == 0) {
		outrec();
		goto out;
	}
	switch(*p) {
	default:
		goto loop2;

	case '1':
	case '4':
		outrec();
		goto loop1;

	case '2':
		side = "   <=";
		goto loop2;

	case '3':
		side = "   =>";
		goto loop2;

	case 'n':
	case 'l':
	case 'y':
	case 'c':
		if(side) {
			if(rec.nname < nelem(rec.name))
				rec.name[rec.nname++] = strdict(side);
			side = 0;
		}
		if(rec.nname < nelem(rec.name))
			rec.name[rec.nname++] = strdict(p+1);
		goto loop2;

	case 'u':
	case 'h':
		if(rec.nlat < nelem(rec.lat))
			rec.lat[rec.nlat++] = atol(p+1);
		rec.scale = 1;
		goto loop2;
	case 't':
	case 'g':
		if(rec.nlat < nelem(rec.lat))
			rec.lat[rec.nlat++] = atol(p+1)*5;
		rec.scale = 0;
		goto loop2;

	case 'd':
		if(rec.nlat < nelem(rec.lat) && rec.nlat > 0)
			if(rec.scale)
				rec.lat[rec.nlat++] = atol(p+1) + rec.lat[rec.nlat-1];
			else
				rec.lat[rec.nlat++] = atol(p+1)*5 + rec.lat[rec.nlat-1];
		goto loop2;
	}

out:
	Tclose();
	if(!init)
		return;

	rscale = (Nscale-1)/2;
	normscales();

	maxlat = -1e4;
	maxlng = -1e4;
	minlat = 1e4;
	minlng = 1e4;
	for(r=reclist; r; r=r->link) {
		l = r->loc;
		dclass = dispclass[rscale];
		for(i=r->nloc; i>0; i--,l++) {
			if(dclass[r->type] == 0)
				continue;
			d = l->lat;
			if(d < minlat)
				minlat = d;
			if(d > maxlat)
				maxlat = d;
			d = l->lng;
			if(d < minlng)
				minlng = d;
			if(d > maxlng)
				maxlng = d;
		}
	}

	if(minlat > maxlat) {
		for(r=reclist; r; r=r->link) {
			l = r->loc;
			for(i=r->nloc; i>0; i--,l++) {
				d = l->lat;
				minlat = d;
				maxlat = d;
				d = l->lng;
				minlng = d;
				maxlng = d;
			}
		}
	}

	loc->lat = (minlat + maxlat) / 2;
	loc->lng = (minlng + maxlng) / 2;

	loc->lat = norm(loc->lat, TWOPI);
	loc->lng = norm(loc->lng, TWOPI);

	loc->sinlat = sin(loc->lat);
	loc->coslat = cos(loc->lat);
}

void
setdclass(void)
{
	int i, j, ty;

	memset(dispclass, 0, sizeof(dispclass));
	memset(saveclass, 0, sizeof(saveclass));

	for(i=0; small[i]; i++) {
		ty = class(small[i]);
		if(ty == 0)
			continue;
		saveclass[ty] = 1;
		for(j=0; j<=8; j++)
			dispclass[j][ty] = 1;
	}

	for(i=0; big[i]; i++) {
		ty = class(big[i]);
		if(ty == 0)
			continue;
		saveclass[ty] = 1;
		for(j=0; j<Nscale; j++)
			dispclass[j][ty] = 1;
	}

	for(i=0; light[i]; i++) {
		ty = class(light[i]);
		if(ty == 0)
			continue;
		lightclass[ty] = 1;
	}

	for(i=0; dark[i]; i++) {
		ty = class(dark[i]);
		if(ty == 0)
			continue;
		darkclass[ty] = 1;
	}
}

double
drop(Loc *a, Loc *b, Loc *c)
{
	double ac, bc, ab, ac2, x;

	ac = dist(a, c);
	bc = dist(b, c);
	ab = dist(a, b);
	if(ac < ab) {
		if(bc < ab) {
			ac2 = ac*ac;
			x = (ac2 - bc*bc + ab*ab) / (2*ab);
			x = ac2 - x*x;
			if(x <= 0)
				return 0;
			return sqrt(x);
		}
		return ac;
	}
	if(bc < ab)
		return bc;
	if(ac < bc)
		return ac;
	return bc;
}

void
lookup(Loc *loc, int but)
{
	double a, d;
	double minlng, maxlng, minlat, maxlat;
	uchar *dclass;
	Rec *r, *br;
	Loc *l;
	int i, j;

	if(0) {
		d = (map.width * map.scale)/(2*RADE);
		minlng = map.loc.lng - d;
		maxlng = map.loc.lng + d;
		d = (map.height * map.scale)/(2*RADE);
		minlat = map.loc.lat - d;
		maxlat = map.loc.lat + d;
	} else {
		d = 2/RADE;
		minlng = loc->lng - d;
		maxlng = loc->lng + d;
		d = 2/RADE;
		minlat = loc->lat - d;
		maxlat = loc->lat + d;
	}

	a = 1e4;
	br = 0;

	if(but == 2)
		goto area;

	dclass = dispclass[rscale];
	for(r=reclist; r; r=r->link) {
		if(dclass[r->type] == 0)
			continue;
		l = r->loc;
		for(i=r->nloc-1; i>0; i--,l++) {
			d = l->lat;
			if(d < minlat || d > maxlat)
				continue;
			d = l->lng;
			if(d < minlng || d > maxlng)
				continue;
			d = drop(l, l+1, loc);
			if(d < a) {
				a = d;
				br = r;
			}
		}
	}
	goto norm;

area:
	for(r=reclist; r; r=r->link) {
		for(i=r->nname-2; i>0; i--)
			if(*r->names[i] == ' ')
				goto afound;
		continue;
	afound:
		l = r->loc;
		for(i=r->nloc-1; i>0; i--,l++) {
			d = drop(l, l+1, loc);
			if(d < a) {
				a = d;
				br = r;
			}
		}
	}
	goto norm;

norm:
	if(br == 0)
		return;

	convcl(w.p1.line[0], br->type);
	for(i=1; i<Nline; i++)
		w.p1.line[i][0] = 0;
	j = 0;
	if(br->nname > 1) {
		for(i=0; i<br->nname; i++) {
			if(*br->names[i] == ' ') {
				j++;
				if(j >= Nline)
					break;
			}
			if(strlen(w.p1.line[j])+strlen(br->names[i]) >= sizeof(w.p1.line[0])-4) {
				j++;
				if(j >= Nline)
					break;
			}
			if(*w.p1.line[j] != 0)
				strcat(w.p1.line[j], "; ");
			strcat(w.p1.line[j], br->names[i]);
		}
	} else
	if(br->nname == 1) {
		strcat(w.p1.line[0], "; ");
		strcat(w.p1.line[0], br->name);
	} else
		strcat(w.p1.line[0], "; <no name>");
	w.loc = *loc;
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
