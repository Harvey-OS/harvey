/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <u.h>
#include <libc.h>
#include <bio.h>
#include <ctype.h>
#include <mach.h>
#define Extern extern
#include "acid.h"
#include "y.tab.h"

static int syren;

Lsym*
unique(char *buf, Sym *s)
{
	Lsym *l;
	int i, renamed;

	renamed = 0;
	strcpy(buf, s->name);
	for(;;) {
		l = look(buf);
		if(l == 0 || (l->lexval == Tid && l->v->set == 0))
			break;

		if(syren == 0 && !quiet) {
			print("Symbol renames:\n");
			syren = 1;
		}
		i = strlen(buf)+1;
		memmove(buf+1, buf, i);
		buf[0] = '$';
		renamed++;
		if(renamed > 5 && !quiet) {
			print("Too many renames; must be X source!\n");
			break;
		}
	}
	if(renamed && !quiet)
		print("\t%s=%s %c/%llux\n", s->name, buf, s->type, s->value);
	if(l == 0)
		l = enter(buf, Tid);
	return l;
}

void
varsym(void)
{
	int i;
	Sym *s;
	int32_t n;
	Lsym *l;
	uint64_t v;
	char buf[1024];
	List *list, **tail, *l2, *tl;

	tail = &l2;
	l2 = 0;

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

			l = unique(buf, s);

			l->v->set = 1;
			l->v->type = TINT;
			l->v->store.ival = v;
			if(l->v->store.comt == 0)
				l->v->store.fmt = 'X';

			/* Enter as list of { name, type, value } */
			list = al(TSTRING);
			tl->store.l = list;
			list->store.string = strnode(buf);
			list->store.fmt = 's';
			list->next = al(TINT);
			list = list->next;
			list->store.fmt = 'c';
			list->store.ival = s->type;
			list->next = al(TINT);
			list = list->next;
			list->store.fmt = 'X';
			list->store.ival = v;

		}
	}
	l = mkvar("symbols");
	l->v->set = 1;
	l->v->type = TLIST;
	l->v->store.l = l2;
	if(l2 == 0)
		print("no symbol information\n");
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
	v->store.l = 0;
	tail = &v->store.l;

	for(r = mach->reglist; r->rname; r++) {
		l = mkvar(r->rname);
		v = l->v;
		v->set = 1;
		v->store.ival = r->roffs;
		v->store.fmt = r->rformat;
		v->type = TINT;

		li = al(TSTRING);
		li->store.string = strnode(r->rname);
		li->store.fmt = 's';
		*tail = li;
		tail = &li->next;
	}

	if(machdata == 0)
		return;

	l = mkvar("bpinst");	/* Breakpoint text */
	v = l->v;
	v->type = TSTRING;
	v->store.fmt = 's';
	v->set = 1;
	v->store.string = gmalloc(sizeof(String));
	v->store.string->len = machdata->bpsize;
	v->store.string->string = gmalloc(machdata->bpsize);
	memmove(v->store.string->string, machdata->bpinst, machdata->bpsize);
}

void
loadvars(void)
{
	Lsym *l;
	Value *v;

	l =  mkvar("proc");
	v = l->v;
	v->type = TINT;
	v->store.fmt = 'X';
	v->set = 1;
	v->store.ival = 0;

	l = mkvar("pid");		/* Current process */
	v = l->v;
	v->type = TINT;
	v->store.fmt = 'D';
	v->set = 1;
	v->store.ival = 0;

	mkvar("notes");			/* Pending notes */

	l = mkvar("proclist");		/* Attached processes */
	l->v->type = TLIST;
}

uint64_t
rget(Map *map, char *reg)
{
	Lsym *s;
	uint32_t x;
	uint64_t v;
	int ret;

	s = look(reg);
	if(s == 0)
		fatal("rget: %s\n", reg);

	switch(s->v->store.fmt){
	default:
		ret = get4(map, s->v->store.ival, &x);
		v = x;
		break;
	case 'V':
	case 'W':
	case 'Y':
	case 'Z':
		ret = get8(map, s->v->store.ival, &v);
		break;
	}
	if(ret < 0)
		error("can't get register %s: %r\n", reg);
	return v;
}

String*
strnodlen(char *name, int len)
{
	String *s;

	s = gmalloc(sizeof(String)+len+1);
	s->string = (char*)s+sizeof(String);
	s->len = len;
	if(name != 0)
		memmove(s->string, name, len);
	s->string[len] = '\0';

	s->gc.gclink = gcl;
	gcl = &s->gc;

	return s;
}

String*
strnode(char *name)
{
	return strnodlen(name, strlen(name));
}

String*
runenode(Rune *name)
{
	int len;
	Rune *p;
	String *s;

	p = name;
	for(len = 0; *p; p++)
		len++;

	len++;
	len *= sizeof(Rune);
	s = gmalloc(sizeof(String)+len);
	s->string = (char*)s+sizeof(String);
	s->len = len;
	memmove(s->string, name, len);

	s->gc.gclink = gcl;
	gcl = &s->gc;

	return s;
}

String*
stradd(String *l, String *r)
{
	int len;
	String *s;

	len = l->len+r->len;
	s = gmalloc(sizeof(String)+len+1);
	s->gc.gclink = gcl;
	gcl = &s->gc;
	s->len = len;
	s->string = (char*)s+sizeof(String);
	memmove(s->string, l->string, l->len);
	memmove(s->string+l->len, r->string, r->len);
	s->string[s->len] = 0;
	return s;
}

String*
straddrune(String *l, Rune r)
{
	int len;
	String *s;

	len = l->len+runelen(r);
	s = gmalloc(sizeof(String)+len+1);
	s->gc.gclink = gcl;
	gcl = &s->gc;
	s->len = len;
	s->string = (char*)s+sizeof(String);
	memmove(s->string, l->string, l->len);
	runetochar(s->string+l->len, &r);
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
