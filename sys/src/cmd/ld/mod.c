#include	"l.h"

void
readundefs(char *f, int t)
{
	int i, n;
	Sym *s;
	Biobuf *b;
	char *l, buf[256], *fields[64];

	if(f == nil)
		return;
	b = Bopen(f, OREAD);
	if(b == nil){
		diag("could not open %s: %r", f);
		errorexit();
	}
	while((l = Brdline(b, '\n')) != nil){
		n = Blinelen(b);
		if(n >= sizeof(buf)){
			diag("%s: line too long", f);
			errorexit();
		}
		memmove(buf, l, n);
		buf[n-1] = '\0';
		n = getfields(buf, fields, nelem(fields), 1, " \t\r\n");
		if(n == nelem(fields)){
			diag("%s: bad format", f);
			errorexit();
		}
		for(i = 0; i < n; i++){
			s = lookup(fields[i], 0);
			s->type = SXREF;
			s->subtype = t;
			if(t == SIMPORT)
				nimports++;
			else
				nexports++;
		}
	}
	Bterm(b);
}

void
undefsym(Sym *s)
{
	int n;

	n = imports;
	if(s->value != 0)
		diag("value != 0 on SXREF");
	if(n >= 1<<Rindex)
		diag("import index %d out of range", n);
	s->value = n<<Roffset;
	s->type = SUNDEF;
	imports++;
}

void
zerosig(char *sp)
{
	Sym *s;

	s = lookup(sp, 0);
	s->sig = 0;
}

void
import(void)
{
	int i;
	Sym *s;

	for(i = 0; i < NHASH; i++)
		for(s = hash[i]; s != S; s = s->link)
			if(s->sig != 0 && s->type == SXREF && (nimports == 0 || s->subtype == SIMPORT)){
				undefsym(s);
				Bprint(&bso, "IMPORT: %s sig=%lux v=%lld\n", s->name, s->sig, (vlong)s->value);
			}
}

void
ckoff(Sym *s, long v)
{
	if(v < 0 || v >= 1<<Roffset)
		diag("relocation offset %ld for %s out of range", v, s->name);
}

static Prog*
newdata(Sym *s, int o, int w, int t)
{
	Prog *p;

	p = prg();
	p->link = datap;
	datap = p;
	p->as = ADATA;
	p->reg = w;
	p->from.type = D_OREG;
	p->from.name = t;
	p->from.sym = s;
	p->from.offset = o;
	p->to.type = D_CONST;
	p->to.name = D_NONE;
	return p;
}

void
export(void)
{
	int i, j, n, off, nb, sv, ne;
	Sym *s, *et, *str, **esyms;
	Prog *p;
	char buf[NSNAME], *t;

	n = 0;
	for(i = 0; i < NHASH; i++)
		for(s = hash[i]; s != S; s = s->link)
			if(s->sig != 0 && s->type != SXREF && s->type != SUNDEF && (nexports == 0 || s->subtype == SEXPORT))
				n++;
	esyms = malloc(n*sizeof(Sym*));
	ne = n;
	n = 0;
	for(i = 0; i < NHASH; i++)
		for(s = hash[i]; s != S; s = s->link)
			if(s->sig != 0 && s->type != SXREF && s->type != SUNDEF && (nexports == 0 || s->subtype == SEXPORT))
				esyms[n++] = s;
	for(i = 0; i < ne-1; i++)
		for(j = i+1; j < ne; j++)
			if(strcmp(esyms[i]->name, esyms[j]->name) > 0){
				s = esyms[i];
				esyms[i] = esyms[j];
				esyms[j] = s;
			}

	nb = 0;
	off = 0;
	et = lookup(EXPTAB, 0);
	if(et->type != 0 && et->type != SXREF)
		diag("%s already defined", EXPTAB);
	et->type = SDATA;
	str = lookup(".string", 0);
	if(str->type == 0)
		str->type = SDATA;
	sv = str->value;
	for(i = 0; i < ne; i++){
		s = esyms[i];
		Bprint(&bso, "EXPORT: %s sig=%lux t=%d\n", s->name, s->sig, s->type);

		/* signature */
		p = newdata(et, off, sizeof(long), D_EXTERN);
		off += sizeof(long);
		p->to.offset = s->sig;

		/* address */
		p = newdata(et, off, sizeof(long), D_EXTERN);
		off += sizeof(long);
		p->to.name = D_EXTERN;
		p->to.sym = s;

		/* string */
		t = s->name;
		n = strlen(t)+1;
		for(;;){
			buf[nb++] = *t;
			sv++;
			if(nb >= NSNAME){
				p = newdata(str, sv-NSNAME, NSNAME, D_STATIC);
				p->to.type = D_SCONST;
				p->to.sval = malloc(NSNAME);
				memmove(p->to.sval, buf, NSNAME);
				nb = 0;
			}
			if(*t++ == 0)
				break;
		}

		/* name */
		p = newdata(et, off, sizeof(long), D_EXTERN);
		off += sizeof(long);
		p->to.name = D_STATIC;
		p->to.sym = str;
		p->to.offset = sv-n;
	}

	if(nb > 0){
		p = newdata(str, sv-nb, nb, D_STATIC);
		p->to.type = D_SCONST;
		p->to.sval = malloc(NSNAME);
		memmove(p->to.sval, buf, nb);
	}

	for(i = 0; i < 3; i++){
		newdata(et, off, sizeof(long), D_EXTERN);
		off += sizeof(long);
	}
	et->value = off;
	if(sv == 0)
		sv = 1;
	str->value = sv;
	exports = ne;
	free(esyms);
}
