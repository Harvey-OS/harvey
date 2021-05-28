#include	<u.h>
#include	<libc.h>
#include	<a.out.h>
#include	<dynld.h>

static ulong
get2(uchar *b)
{
	return (b[0] << 8) | b[1];
}

static ulong
get4(uchar *b)
{
	return (b[0] << 24) | (b[1] << 16) | (b[2] << 8) | b[3];
}

static ulong
lgetbe(ulong l)
{
	union {
		ulong	 l;
		uchar	c[4];
	} u;
	u.l = l;
	return get4(u.c);
}

Dynsym*
dynfindsym(char *s, Dynsym *tab, int ntab)
{
	int n, n2, d;
	Dynsym *t, *m;

	t = tab;
	n = ntab;
	while(n > 0){
		n2 = n>>1;
		m = t+n2;
		d = strcmp(s, m->name);
		if(d < 0){
			n = n2;
			continue;
		}
		if(d > 0){
			t = m+1;
			n -= n2+1;
			continue;
		}
		return m;
	}
	return nil;
}

void*
dynimport(Dynobj *o, char *name, ulong sig)
{
	Dynsym *t;

	t = dynfindsym(name, o->export, o->nexport);
	if(t == nil || sig != 0 && t->sig != 0 && t->sig != sig)
		return nil;
	return (void*)t->addr;
}

int
dyntabsize(Dynsym *t)
{
	int n;

	for(n = 0; t->name != nil; t++)
		n++;
	return n;
}

void
dynobjfree(Dynobj *o)
{
	if(o != nil){
		free(o->base);
		free(o->import);
		free(o);
	}
}

void
dynfreeimport(Dynobj *o)
{
	free(o->import);
	o->import = nil;
	o->nimport = 0;
}

static char Ereloc[] = "error reading object file";

Dynobj*
dynloadgen(void *file, long (*rd)(void*,void*,long), vlong (*sk)(void*,vlong,int), void (*werr)(char*), Dynsym *tab, int ntab, ulong maxsize)
{
	int i, m, n, ni, nr, relsize;
	ulong syms, entry, sig, p, a;
	uchar *base;
	Exec e;
	Dynsym *t;
	Dynobj *l;
	char *s, *err, buf[64];
	uchar *reldata, *rp, *ep;
	vlong off;

	err = Ereloc;	/* default */
	off = (*sk)(file, 0, 1);
	l = mallocz(sizeof(Dynobj), 1);
	if(l == nil){
		err = "can't allocate Dynobj";
		goto Error;
	}
	if((*rd)(file, &e, sizeof(Exec)) != sizeof(Exec))
		goto Error;
	if(lgetbe(e.magic) != dynmagic()){
		err = "not dynamic object file or wrong platform";
		goto Error;
	}
	l->text = lgetbe(e.text);
	l->data = lgetbe(e.data);
	l->bss = lgetbe(e.bss);
	syms = lgetbe(e.syms)+lgetbe(e.spsz)+lgetbe(e.pcsz);
	entry = lgetbe(e.entry);
	l->size = l->text + l->data + l->bss;
	if(entry < 0 || entry >= l->size || entry & 3){
		err = "invalid export table pointer (entry point)";
		goto Error;
	}
	if(maxsize && l->size >= maxsize){
		snprint(buf, sizeof(buf), "%lud: object too big", l->size);
		err = buf;
		goto Error;
	}

	l->base = base = malloc(l->size);
	if(base == nil){
		err = "out of memory: loading object file";
		goto Error;
	}
	l->export = (Dynsym*)(base+entry);
	if((*rd)(file, base, l->text+l->data) != l->text+l->data)
		goto Error;
	memset(base+l->text+l->data, 0, l->bss);
	if((*sk)(file, syms, 1) < 0)
		goto Error;
	if((*rd)(file, buf, 4) != 4)
		goto Error;
	relsize = get4((uchar*)buf);	/* always contains at least an import count (might be zero) */
	if(relsize < 4)
		goto Error;
	reldata = malloc(relsize);
	if(reldata == nil){
		err = "out of memory: relocation data";
		goto Error;
	}
	if((*rd)(file, reldata, relsize) != relsize)
		goto Error;
	rp = reldata;
	ep = reldata+relsize;
	ni = get4(rp);
	rp += 4;
	if(ni < 0 || ni > 8000)
		goto Error;	/* implausible size */
	l->nimport = ni;
	l->import = malloc(ni*sizeof(Dynsym*));
	if(l->import == nil){
		err = "out of memory: symbol table";
		goto Error;
	}
	for(i = 0; i < ni; i++){
		if(rp+5 > ep)
			goto Error;
		sig = get4(rp);
		rp += 4;
		s = (char*)rp;
		while(*rp++)
			if(rp >= ep)
				goto Error;
		t = dynfindsym(s, tab, ntab);
		if(t == nil){
			snprint(buf, sizeof(buf), "undefined symbol: %s", s);
			err = buf;
			goto Error;
		}
		if(sig != 0 && t->sig != 0 && t->sig != sig){
			snprint(buf, sizeof(buf), "signature mismatch: %s (%lux != %lux)", s, sig, t->sig);
			err = buf;
			goto Error;
		}
		l->import[i] = t;
	}

	a = 0;
	if(rp+4 > ep)
		goto Error;
	nr = get4(rp);
	rp += 4;
	for(i = 0; i < nr; i++){
		if(rp >= ep)
			goto Error;
		m = *rp++;
		n = m>>6;
		if(rp+(1<<n) > ep)
			goto Error;
		switch(n){
		case 0:
			p = *rp++;
			break;
		case 1:
			p = get2(rp);
			rp += 2;
			break;
		case 2:
			p = get4(rp);
			rp += 4;
			break;
		default:
			goto Error;
		}
		a += p;
		err = dynreloc(base, a, m&0xf, l->import, ni);
		if(err != nil){
			snprint(buf, sizeof(buf), "dynamic object: %s", err);
			err = buf;
			goto Error;
		}
	}
	free(reldata);

	/* could check relocated export table here */
	l->nexport = dyntabsize(l->export);

	segflush(base, l->text);

	return l;

Error:
	if(off >= 0)
		(*sk)(file, off, 0);	/* restore original file offset */
	(*werr)(err);
	dynobjfree(l);
	return nil;
}

int
dynloadable(void* file, long (*rd)(void*,void*,long), vlong (*sk)(void*,vlong,int))
{
	long magic;

	if((*rd)(file, &magic, sizeof(magic)) != sizeof(magic)){
		(*sk)(file, -(signed int)sizeof(magic), 1);
		return 0;
	}
	(*sk)(file, -(signed int)sizeof(magic), 1);
	return lgetbe(magic) == dynmagic();
}
