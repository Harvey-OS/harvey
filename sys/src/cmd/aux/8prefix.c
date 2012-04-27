/*
 * Pre-resolve references inside an object file.
 * Mark such functions static so that linking with
 * other object files can't get at them.
 * Also rename "main".
 */
#include <u.h>
#include <libc.h>
#include <bio.h>
#include "/sys/src/cmd/8c/8.out.h"

typedef struct Sym Sym;
struct Sym
{
	char *name;
	char *newname;
	short type;
	short version;
	Sym *link;
};

typedef struct Obj Obj;
struct Obj
{
	int fd;
	int version;
	uchar *bp;
	uchar *ep;
	char *name;
};

enum
{
	NHASH = 10007
};

Sym *hash[NHASH];
int nsymbol;
int renamemain = 1;
Sym *xsym[256];
int version = 1;
Obj **obj;
int nobj;
Biobuf bout;
char *prefix;
int verbose;

void *emalloc(ulong);
Sym *lookup(char*, int);
Obj *openobj(char*);
void walkobj(Obj*, void (*fn)(int, Sym*, uchar*, int));
void walkobjs(void (*fn)(int, Sym*, uchar*, int));
void dump(int, Sym*, uchar*, int);
void nop(int, Sym*, uchar*, int);
void owrite(int, Sym*, uchar*, int);
int zaddr(uchar*, Sym**);
void renamesyms(int, Sym*, uchar*, int);

void
usage(void)
{
	fprint(2, "usage: 8prelink [-mv] prefix file.8...\n");
	exits("usage");
}

void
main(int argc, char **argv)
{
	int i;
	Obj *o;

	ARGBEGIN{
	case 'm':
		renamemain = 0;
		break;
	case 'v':
		verbose = 1;
		break;
	default:
		usage();
	}ARGEND
	
	if(argc < 2)
		usage();
	
	prefix = argv[0];
	argv++;
	argc--;

	nobj = argc;
	obj = emalloc(nobj*sizeof obj[0]);
	for(i=0; i<argc; i++)
		obj[i] = openobj(argv[i]);

	walkobjs(nop);	/* initialize symbol table */
	if(verbose)
		walkobjs(dump);
	walkobjs(renamesyms);
	
	for(i=0; i<nobj; i++){
		o = obj[i];
		seek(o->fd, 0, 0);
		Binit(&bout, o->fd, OWRITE);
		walkobj(o, owrite);
		Bflush(&bout);
	}
	exits(0);
}

void
renamesyms(int op, Sym *sym, uchar*, int)
{
	if(sym && sym->version==0 && !sym->newname)
	switch(op){
	case AGLOBL:
	case AINIT:
	case ADATA:
	case ATEXT:
		if(!renamemain && strcmp(sym->name, "main") == 0)
			break;
		sym->newname = smprint("%s%s", prefix, sym->name);
		break;
	}	
}

void
dump(int op, Sym *sym, uchar*, int)
{
	if(sym && sym->version==0)
	switch(op){
	case AGLOBL:
	case AINIT:
	case ADATA:
	case ATEXT:
		print("%s\n", sym->name);
		break;
	}	
}

void
nop(int, Sym*, uchar*, int)
{
}

void
owrite(int op, Sym *sym, uchar *p, int l)
{
	switch(op){
	case ASIGNAME:
		Bwrite(&bout, p, 4);
		p += 4;
		l -= 4;
	case ANAME:
		if(sym->newname){
			Bwrite(&bout, p, 4);
			Bwrite(&bout, sym->newname, strlen(sym->newname)+1);
			break;
		}
	default:
		Bwrite(&bout, p, l);
		break;
	}
}

int
zaddr(uchar *p, Sym **symp)
{
	int c, t;
	
	t = p[0];
	c = 1;
	if(t & T_INDEX)
		c += 2;
	if(t & T_OFFSET)
		c += 4;
	if(t & T_SYM){
		if(symp)
			*symp = xsym[p[c]];
		c++;
	}
	if(t & T_FCONST)
		c += 8;
	else if(t & T_SCONST)
		c += NSNAME;
	if(t & T_TYPE)
		c++;
	return c;
}

void*
emalloc(ulong n)
{
	void *v;
	
	v = mallocz(n, 1);
	if(v == nil)
		sysfatal("out of memory");
	return v;
}

Sym*
lookup(char *symb, int v)
{
	Sym *s;
	char *p;
	long h;
	int l, c;

	h = v;
	for(p=symb; c = *p; p++)
		h = h+h+h + c;
	l = (p - symb) + 1;
	if(h < 0)
		h = ~h;
	h %= NHASH;
	for(s = hash[h]; s != nil; s = s->link)
		if(s->version == v)
		if(memcmp(s->name, symb, l) == 0)
			return s;

	s = emalloc(sizeof *s);
	s->name = emalloc(l + 1);
	memmove(s->name, symb, l);

	s->link = hash[h];
	s->type = 0;
	s->version = v;
	hash[h] = s;
	nsymbol++;
	return s;
}

Obj*
openobj(char *name)
{
	Dir *d;
	Obj *obj;
	
	obj = emalloc(sizeof *obj);
	obj->name = name;
	obj->version = version++;
	if((obj->fd = open(name, ORDWR)) < 0)
		sysfatal("open %s: %r", name);
	if((d = dirfstat(obj->fd)) == nil)
		sysfatal("dirfstat: %r");
	obj->bp = emalloc(d->length);
	if(readn(obj->fd, obj->bp, d->length) != d->length)
		sysfatal("read %s: %r", name);
	obj->ep = obj->bp+d->length;
	return obj;
}

void
walkobjs(void (*fn)(int, Sym*, uchar*, int))
{
	int i;
	
	for(i=0; i<nobj; i++)
		walkobj(obj[i], fn);
}

void
walkobj(Obj *obj, void (*fn)(int, Sym*, uchar*, int))
{
	int op, type;
	Sym *sym;
	uchar *p, *p0;

	for(p=obj->bp; p+4<=obj->ep; ){
		op = p[0] | (p[1]<<8);
		if(op <= AXXX || op >= ALAST)
			sysfatal("%s: opcode out of range - probably not a .8 file", obj->name);
		p0 = p;
		switch(op){
		case ASIGNAME:
			p += 4;	/* sign */
		case ANAME:
			type = p[2];
			sym = lookup((char*)p+4, type==D_STATIC ? obj->version : 0);
			xsym[p[3]] = sym;
			p += 4+strlen(sym->name)+1;
			fn(op, sym, p0, p-p0);
			break;
		
		default:
			p += 6;
			p += zaddr(p, &sym);
			p += zaddr(p, nil);
			fn(op, sym, p0, p-p0);
			break;
		}
	}
}

