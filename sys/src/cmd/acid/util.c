#include <u.h>
#include <libc.h>
#include <bio.h>
#include <ctype.h>
#include <mach.h>
#define Extern extern
#include "acid.h"
#include "y.tab.h"

static int syren;

void
varsym(void)
{
	int i;
	Sym *s;
	long n;
	Lsym *l;
	ulong v;
	char buf[NNAME+1];
	List *list, **tail, *l2, *tl;

	tail = &l2;

	symbase(&n);
	for(i = 0; i < n; i++) {
		s = getsym(i);
		switch(s->type) {
		case 'T':
		case 'L':
		case 'D':
		case 'B':
		case 'b':
		case 'd':
		case 'l':
		case 't':
			if(s->name[0] == '.')
				continue;

			v = s->value;
			tl = al(TLIST);
			*tail = tl;
			tail = &tl->next;

			l = look(s->name);
			if(l != 0 && (l->v->set || l->lexval != Tid)) {
				if(syren == 0) {
					print("Symbol renames:\n");
					syren = 1;
				}
				buf[0] = '$';
				strcpy(buf+1, s->name);
				print("\t%s=%s %c/0x%lux\n",
						s->name, buf, s->type);	
			}
			else
				strcpy(buf, s->name);

			if(l == 0)
				l = enter(buf, Tid);
			l->v->set = 1;
			l->v->type = TINT;
			l->v->ival = v;
			l->v->fmt = 'X';

			/* Enter as list of { name, type, value } */
			list = al(TSTRING);
			tl->l = list;
			list->string = strnode(buf);
			list->fmt = 's';
			list->next = al(TINT);
			list = list->next;
			list->fmt = 'c';
			list->ival = s->type;
			list->next = al(TINT);
			list = list->next;
			list->fmt = 'X';
			list->ival = v;

		}
	}
	l = mkvar("symbols");
	l->v->set = 1;
	l->v->type = TLIST;
	l->v->l = l2;	
}

void
varreg(void)
{
	Lsym *l;
	Value *v;
	Reglist *r;
	List **tail, *li;

	l = mkvar("registers");
	v = l->v;
	v->set = 1;
	v->type = TLIST;
	v->l = 0;
	tail = &v->l;

	for(r = mach->reglist; r->rname; r++) {
		l = mkvar(r->rname);
		v = l->v;
		v->set = 1;
		v->ival = mach->kbase+r->roffs;
		v->fmt = r->rformat;
		v->type = TINT;

		li = al(TSTRING);
		li->string = strnode(r->rname);
		li->fmt = 's';
		*tail = li;
		tail = &li->next;
	}

	l = mkvar("pid");		/* Current process */
	v = l->v;
	v->type = TINT;
	v->fmt = 'D';
	v->set = 1;
	v->ival = 0;

	mkvar("notes");		/* Pending notes */
	l = mkvar("proclist");	/* Attached processes */
	l->v->type = TLIST;

	if(machdata == 0)
		return;

	l = mkvar("bpinst");	/* Breakpoint text */
	v = l->v;
	v->type = TSTRING;
	v->fmt = 's';
	v->set = 1;
	v->string = gmalloc(sizeof(String));
	v->string->len = machdata->bpsize;
	v->string->string = gmalloc(machdata->bpsize);
	memmove(v->string->string, machdata->bpinst, machdata->bpsize);
}

String*
strnode(char *name)
{
	int len;
	String *s;

	len = strlen(name);
	s = gmalloc(sizeof(String)+len+1);
	s->string = (char*)s+sizeof(String);
	s->len = len;
	strcpy(s->string, name);

	s->gclink = gcl;
	gcl = s;

	return s;
}

String*
stradd(String *l, String *r)
{
	int len;
	String *s;

	len = l->len+r->len;
	s = gmalloc(sizeof(String)+len+1);
	s->gclink = gcl;
	gcl = s;
	s->len = len;
	s->string = (char*)s+sizeof(String);
	memmove(s->string, l->string, l->len);
	memmove(s->string+l->len, r->string, r->len);
	s->string[s->len] = 0;
	return s;
}

int
scmp(String *sr, String *sl)
{
	if(sr->len != sl->len)
		return 0;

	if(memcmp(sr->string, sl->string, sl->len))
		return 0;

	return 1;
}
