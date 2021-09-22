#include "l.h"

enum{
	ABSD = 0,
	ABSU = 1,
	RELD = 2,
	RELU = 3,
};

int modemap[4] = { 0, 1, -1, 2, };

typedef struct Reloc Reloc;

struct Reloc
{
	int n;
	int t;
	uchar *m;
	ulong *a;
};

Reloc rels;

static void
grow(Reloc *r)
{
	int t;
	uchar *m, *nm;
	ulong *a, *na;

	t = r->t;
	r->t += 64;
	m = r->m;
	a = r->a;
	r->m = nm = malloc(r->t*sizeof(uchar));
	r->a = na = malloc(r->t*sizeof(ulong));
	memmove(nm, m, t*sizeof(uchar));
	memmove(na, a, t*sizeof(ulong));
	free(m);
	free(a);
}

void
dynreloc(Sym *s, long v, int abs)
{
	int i, k, n;
	uchar *m;
	ulong *a;
	Reloc *r;

	if(v&3)
		diag("bad relocation address");
	v >>= 2;
	if(s != S && s->type == SUNDEF)
		k = abs ? ABSU : RELU;
	else
		k = abs ? ABSD : RELD;
	/* Bprint(&bso, "R %s a=%ld(%lx) %d\n", s->name, a, a, k); */
	k = modemap[k];
	r = &rels;
	n = r->n;
	if(n >= r->t)
		grow(r);
	m = r->m;
	a = r->a;
	for(i = n; i > 0; i--){
		if(v < a[i-1]){	/* happens occasionally for data */
			m[i] = m[i-1];
			a[i] = a[i-1];
		}
		else
			break;
	}
	m[i] = k;
	a[i] = v;
	r->n++;
}

static int
sput(char *s)
{
	char *p;

	p = s;
	while(*s)
		cput(*s++);
	cput(0);
	return  s-p+1;
}

void
asmdyn(void)
{
	int i, n, t, c;
	Sym *s;
	ulong la, ra, *a;
	vlong off;
	uchar *m;
	Reloc *r;

	cflush();
	off = seek(cout, 0, 1);
	lput(0);
	t = 0;
	lput(imports);
	t += 4;
	for(i = 0; i < NHASH; i++)
		for(s = hash[i]; s != S; s = s->link)
			if(s->type == SUNDEF){
				lput(s->sig);
				t += 4;
				t += sput(s->name);
			}
	
	la = 0;
	r = &rels;
	n = r->n;
	m = r->m;
	a = r->a;
	lput(n);
	t += 4;
	for(i = 0; i < n; i++){
		ra = *a-la;
		if(*a < la)
			diag("bad relocation order");
		if(ra < 256)
			c = 0;
		else if(ra < 65536)
			c = 1;
		else
			c = 2;
		cput((c<<6)|*m++);
		t++;
		if(c == 0){
			cput(ra);
			t++;
		}
		else if(c == 1){
			wput(ra);
			t += 2;
		}
		else{
			lput(ra);
			t += 4;
		}
		la = *a++;
	}

	cflush();
	seek(cout, off, 0);
	lput(t);

	if(debug['v']){
		Bprint(&bso, "import table entries = %d\n", imports);
		Bprint(&bso, "export table entries = %d\n", exports);
	}
}
