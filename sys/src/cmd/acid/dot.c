#include <u.h>
#include <libc.h>
#include <bio.h>
#include <ctype.h>
#include <mach.h>
#define Extern extern
#include "acid.h"

struct maptype
{
	char	*name;
	char	fmt;
} maptype[] = {
	"CHAR",		'b',
	"UCHAR",	'b',
	"SHORT",	'd',
	"USHORT",	'u',
	"LONG",		'D',
	"ULONG",	'U',
	"VLONG",	'V',
	"FLOAT",	'f',
	"DOUBLE",	'F',
	"IND",		'X',
	"FUNC",		'X',
	"ARRAY",	'a',
	"STRUCT",	'a',
	"UNION",	'a',
	0
};

Type*
srch(Type *t, char *s)
{
	while(t) {
		if(strcmp(t->name, s) == 0)
			return t;
		t = t->next;
	}
	return 0;
}

void
dodot(Node *n, Node *r)
{
	char *s;
	Type *t;
	Node res;
	ulong addr;

	USED(n,r);
	s = n->sym->name;

	expr(n->left, &res);
	if(res.comt == 0)
		error("no type specified for (expr).%s", s);

	t = srch(res.comt, s);
	if(t == 0)
		error("no tag for (expr).%s", s);

	if(res.type != TINT)
		error("pointer must be integer for (expr).%s", s);

	addr = res.ival+t->offset;
	if(t->fmt == 'a') {
		r->fmt = 'a';
		r->type = TINT;
		r->ival = addr;
	}
	else
		indir(cormap, addr, t->fmt, r);
}

void
decl(Lsym *type, Lsym *var)
{
	if(type->lt == 0)
		error("%s is not a structure tag", type->name);

	var->v->comt = type->lt;
}

Type*
rdtags(Biobuf *bp)
{
	int i;
	char *p, *nr, *ty;
	Type *t, *lst, **tail;

	lst = 0;
	tail = &lst;

	for(;;) {
		p = Brdline(bp, '\n');
		if(p == 0)
			return lst;
		p[BLINELEN(bp)-1] = '\0';
		nr = strchr(p, ' ');
		if(nr == 0)
			goto bad;
		*nr++ = '\0';
		ty = strchr(nr, ' ');
		if(ty == 0)
			goto bad;
		*ty++ = '\0';

		t = gmalloc(sizeof(Type));
		memset(t, 0, sizeof(Type));
		strncpy(t->name, p, sizeof(t->name));
		strncpy(t->type, ty, sizeof(t->type));
		t->offset = atoi(nr);
		p = strchr(ty, ' ');
		if(p != 0)
			*p = '\0';

		/* Use primary type to invent a format */
		for(i = 0; maptype[i].name; i++) {
			if(strncmp(maptype[i].name, ty, strlen(maptype[i].name)) == 0) {
				t->fmt = maptype[i].fmt;
				break;
			}
		}

		*tail = t;
		tail = &t->next;
	}
bad:
	for(t = lst; t; t = lst) {
		lst = t->next;
		free(t);
	}
	Bclose(bp);
	error("bad tag file format");
	return 0;
}

void
ltag(char *file)
{
	Lsym *l;
	Biobuf *bp;
	char *nr, *p;

	bp = Bopen(file, OREAD);
	if(bp == 0)
		error("tag: open %s: %r", file);

	p = Brdline(bp, '\n');
	if(p == 0) {
		Bclose(bp);
		return;
	}
	p[BLINELEN(bp)-1] = '\0';
	nr = strchr(p, ' ');
	if(nr == 0) {
		error("bad tag file format");
		Bclose(bp);
		return;
	}
	*nr = '\0';

	l = mkvar(p);
	l->lt = rdtags(bp);
	Bprint(bout, "%s\n", l->name);
	Bclose(bp);
}
