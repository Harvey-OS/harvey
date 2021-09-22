#include "jep.h"

void
prparam(Biobuf *b, Param *p)
{
	int i;

	for(i=0; i<nelem(p->param); i++)
		if(p->paramf & (1<<i))
			Bprint(b, " %s-%ux", pname[i], p->param[i]);
}

void
prnode(uchar*)
{
	int i, c;
	Node *n, *l;
	Biobuf *b;

	b = Bopen("/tmp/jepout.pp", OWRITE);
	if(b == 0) {
		print("cant create jepout.pp\n");
		return;
	}
	Bprint(b, "%s\n", h.name);
	if(h.nencap) {
		for(i=0; i<h.nencap; i++) {
			Bprint(b, "encap #%d (gok=%.4x)\n", i, h.encap[i].gok);
			for(c=0; c<h.encap[i].ndata; c++) {
				if(c%16 == 0)
					Bprint(b, "  %.4x", c);
				Bprint(b, " %.2x", h.encap[i].data[c]);
				if(c%16 == 15)
					Bprint(b, "\n");
			}
			if(c%16 != 0)
				Bprint(b, "\n");
		}
		Bprint(b, "\n");
	}
	for(n=root; n; n = n->link) {
		Bprint(b, "%d %.4lux", n->ord, n->beg);
		switch(n->type) {
		default:
			Bprint(b, " type(%d)", n->type);
			break;
		case Tstring:
			Bprint(b, " S (%d,%d) \"%s\"", n->s.x, n->s.y, n->s.str);
			break;
		case Tpoint:
			Bprint(b, " P");
			Bprint(b, " (%d,%d)", n->l.vec[0].x, n->l.vec[0].y);
			break;
		case Tline1:
		case Tline2:
		case Tline3:
		case Tline4:
			Bprint(b, " L%d", n->type-Tline1+1);
			for(i=0; i<2; i++)
				Bprint(b, " (%d,%d)", n->l.vec[i].x, n->l.vec[i].y);
			break;
		case Tcirc1:
		case Tcirc2:
		case Tcirc3:
			Bprint(b, " C%d %d %d %d", n->type-Tcirc1+1, n->c.x, n->c.y, n->c.r);
			break;
		case Tarc:
			Bprint(b, " A %d %d %d %d %d", n->c.x, n->c.y, n->c.r, n->c.a1, n->c.a2);
			break;
		case Tgroup1:
		case Tgroup2:
		case Tgroup3:
			Bprint(b, " G%d %d", n->type-Tgroup1+1, n->g.count);
			if(n->g.name)
				Bprint(b, " \"%s\"", n->g.name);
			break;
		case Tvect1:
		case Tvect2:
		case Tvect3:
			c = n->v.count;
			Bprint(b, " V%d %d", n->type-Tvect1+1, n->v.count);
			if(c > 3)
				c = 3;
			for(i=0; i<c; i++)
				Bprint(b, " (%d,%d)", n->v.vec[i].x, n->v.vec[i].y);
			break;
		}
		prparam(b, &n->p);
		Bprint(b, "\n");
		for(l=n->parent; l; l=l->parent) {
			Bprint(b, "	%4d G%d", l->ord, l->type-Tgroup1+1);
			prparam(b, &l->p);
			Bprint(b, " ||");
			prparam(b, &l->g.p);
			Bprint(b, "\n");
		}
	}
	Bterm(b);
}

int
hex(int v)
{
	v &= 0xf;
	if(v < 10)
		return v+'0';
	return v + ('a'-10);
}

char*
aparam(char *q, Param *p)
{
	int i, v;

	*q++ = '-';
	for(i=0; i<=Px; i++) {
		if(p->paramf & (1<<i)) {
			if("121121211"[i] == '1') {
				v = p->param[i];
				*q++ = hex(v>>4);
				*q++ = hex(v>>0);
			} else {
				v = p->param[i];
				*q++ = hex(v>>12);
				*q++ = hex(v>>8);
				*q++ = hex(v>>4);
				*q++ = hex(v>>0);
			}
		} else {
			if("121121211"[i] == '1') {
				*q++ = 'X';
				*q++ = 'X';
			} else {
				*q++ = 'X';
				*q++ = 'X';
				*q++ = 'X';
				*q++ = 'X';
			}
		}
	}
	return q;
}

char*
aprint(Node *l)
{
	char *p;

	p = apbuf;
	switch(l->type) {
	default:
		*p++ = 'X';
		*p++ = 'X';
		break;
	case Tstring:
		*p++ = 'S';
		*p++ = '0';
		break;
	case Tpoint:
		*p++ = 'P';
		*p++ = '0';
		break;
	case Tline1:
	case Tline2:
	case Tline3:
	case Tline4:
		*p++ = 'L';
		*p++ = l->type + ('1'-Tline1);
		break;
	case Tcirc1:
	case Tcirc2:
	case Tcirc3:
		*p++ = 'C';
		*p++ = l->type + ('1'-Tcirc1);
		break;
	case Tarc:
		*p++ = 'A';
		*p++ = '0';
		break;
	case Tgroup1:
	case Tgroup2:
	case Tgroup3:
		*p++ = 'G';
		*p++ = l->type + ('1'-Tgroup1);
		break;
	case Tvect1:
	case Tvect2:
	case Tvect3:
		*p++ = 'V';
		*p++ = l->type + ('1'-Tvect1);
		break;
	}
	p = aparam(p, &l->p);
	for(l=l->parent; l; l=l->parent) {
		*p++ = '\n';
		*p++ = '\t';
		p = aparam(p, &l->p);
		p = aparam(p, &l->g.p);
	}
	*p++ = '%';
	*p = 0;
	return apbuf;
}

void
allnode(uchar*)
{
	Node *n;
	Biobuf *b;

	b = Bopen("/tmp/jepout.pp", OWRITE);
	if(b == 0) {
		print("cant create jepout.pp\n");
		return;
	}
	for(n=root; n; n = n->link)
		Bprint(b, "%3d %s\n", n->ord, aprint(n));
	Bterm(b);
}
