#include <u.h>
#include <libc.h>
#include <bio.h>
#include <auth.h>
#include "imap4d.h"

/*
 * iterated over all of the items in the message set.
 * errors are accumulated, but processing continues.
 * if uids, then ignore non-existent messages.
 * otherwise, that's an error
 */
int
forMsgs(Box *box, MsgSet *ms, ulong max, int uids, int (*f)(Box*, Msg*, int, void*), void *rock)
{
	Msg *m;
	ulong id;
	int ok, rok;

	ok = 1;
	for(; ms != nil; ms = ms->next){
		id = ms->from;
		rok = 0;
		for(m = box->msgs; m != nil && m->seq <= max; m = m->next){
			if(!uids && m->seq > id
			|| uids && m->uid > ms->to)
				break;
			if(!uids && m->seq == id
			|| uids && m->uid >= id){
				if(!(*f)(box, m, uids, rock))
					ok = 0;
				if(uids)
					id = m->uid;
				if(id >= ms->to){
					rok = 1;
					break;
				}
				if(ms->to == ~0UL)
					rok = 1;
				id++;
			}
		}
		if(!uids && !rok)
			ok = 0;
	}
	return ok;
}

Store *
mkStore(int sign, int op, int flags)
{
	Store *st;

	st = binalloc(&parseBin, sizeof(Store), 1);
	if(st == nil)
		parseErr("out of memory");
	st->sign = sign;
	st->op = op;
	st->flags = flags;
	return st;
}

Fetch *
mkFetch(int op, Fetch *next)
{
	Fetch *f;

	f = binalloc(&parseBin, sizeof(Fetch), 1);
	if(f == nil)
		parseErr("out of memory");
	f->op = op;
	f->next = next;
	return f;
}

Fetch*
revFetch(Fetch *f)
{
	Fetch *last, *next;

	last = nil;
	for(; f != nil; f = next){
		next = f->next;
		f->next = last;
		last = f;
	}
	return last;
}

NList*
mkNList(ulong n, NList *next)
{
	NList *nl;

	nl = binalloc(&parseBin, sizeof(NList), 0);
	if(nl == nil)
		parseErr("out of memory");
	nl->n = n;
	nl->next = next;
	return nl;
}

NList*
revNList(NList *nl)
{
	NList *last, *next;

	last = nil;
	for(; nl != nil; nl = next){
		next = nl->next;
		nl->next = last;
		last = nl;
	}
	return last;
}

SList*
mkSList(char *s, SList *next)
{
	SList *sl;

	sl = binalloc(&parseBin, sizeof(SList), 0);
	if(sl == nil)
		parseErr("out of memory");
	sl->s = s;
	sl->next = next;
	return sl;
}

SList*
revSList(SList *sl)
{
	SList *last, *next;

	last = nil;
	for(; sl != nil; sl = next){
		next = sl->next;
		sl->next = last;
		last = sl;
	}
	return last;
}

int
BNList(Biobuf *b, NList *nl, char *sep)
{
	char *s;
	int n;

	s = "";
	n = 0;
	for(; nl != nil; nl = nl->next){
		n += Bprint(b, "%s%lud", s, nl->n);
		s = sep;
	}
	return n;
}

int
BSList(Biobuf *b, SList *sl, char *sep)
{
	char *s;
	int n;

	s = "";
	n = 0;
	for(; sl != nil; sl = sl->next){
		n += Bprint(b, "%s", s);
		n += Bimapstr(b, sl->s);
		s = sep;
	}
	return n;
}

int
Bimapdate(Biobuf *b, Tm *tm)
{
	char buf[64];

	if(tm == nil)
		tm = localtime(time(nil));
	imap4date(buf, sizeof(buf), tm);
	return Bimapstr(b, buf);
}

int
Brfc822date(Biobuf *b, Tm *tm)
{
	char buf[64];

	if(tm == nil)
		tm = localtime(time(nil));
	rfc822date(buf, sizeof(buf), tm);
	return Bimapstr(b, buf);
}

int
Bimapstr(Biobuf *b, char *s)
{
	char *t;
	int c;

	if(s == nil)
		return Bprint(b, "NIL");
	for(t = s; ; t++){
		c = *t;
		if(c == '\0')
			return Bprint(b, "\"%s\"", s);
		if(t - s > 64 || c >= 0x7f || strchr("\"\\\r\n", c) != nil)
			break;
	}
	return Bprint(b, "{%lud}\r\n%s", strlen(s), s);
}
