#include <u.h>
#include <libc.h>
#include <thread.h>
#include <draw.h>
#include <mouse.h>
#include <keyboard.h>
#include <control.h>
#include <editcontrol.h>
#include "string.h"

static struct {
	char *name;
	Stringset *ss;
} *stringsets;
int nstringset;

static String *	stringcow(String *s);	// cow: copy on write
static Undo *	_newundo(Stringset *ss);

Stringset *
stringsetnamed(char *name)
{
	int i;

	for (i = 0; i < nstringset; i++)
		if (strcmp(stringsets[i].name, name) == 0)
			return(stringsets[i].ss);
	return nil;
}

Stringset *
newstringset(char *name)
{
	Stringset *ss;

	if (stringsetnamed(name))
		ctlerror("namestringset: %s exists", name);
	ss = ctlmalloc(sizeof(Stringset));
	ss->string = nil;
	ss->nstring = 0;
	ss->nalloc = 0;
	ss->snarf = nil;
	ss->nsnarf = 0;
	ss->undostate = &ss->undohome;
	ss->undohome.next = nil;
	ss->undohome.prev = nil;
	stringsets = ctlrealloc(stringsets, (nstringset+1)*sizeof(stringsets[0]));
	stringsets[nstringset].ss = ss;
	stringsets[nstringset++].name = strdup(name);
	return ss;
}

void
freestringset(Stringset *ss)
{
	int i;
	Undo *u, *unext;

	if (ss == nil)
		return;
	for (i = 0; i < ss->nstring; i++)
		freestring(ss->string[i]);
	free(ss->string);
	for (i = 0; i < ss->nsnarf; i++)
		freestring(ss->snarf[i]);
	free(ss->snarf);
	for (u = ss->undohome.next; u; u = unext){
		freestring(u->s);
		unext = u->next;
		free(u);
	}
	free(ss);
}

void
snarfclear(Stringset *ss)
{
	int i;

	// remove old snarf buffer
	for (i = 0; i < ss->nsnarf; i++)
		freestring(ss->snarf[i]);
	ss->nsnarf = 0;
}

String*
stringof(Stringset *ss, Position pos)
{
	if (ss->string == nil || pos.str < 0 || pos.str >= ss->nstring)
		return nil;
	return ss->string[pos.str];
}

Context *
_contextnamed(char *s)
{
	Context *c;

	for (c = context; c; c = c->next)
		if (strcmp(s, c->name) == 0)
			return c;
	return nil;
}

int
_posbefore(Position p1, Position p2)
{
	return p1.str < p2.str || (p1.str == p2.str && p1.rn < p2.rn);
}

int
_posequal(Position p1, Position p2)
{
	return p1.str == p2.str && p1.rn == p2.rn;
}

int
_posdec(Stringset *ss, Position *pp)
{
	String *s;
	Position p;

	p = *pp;
	while (p.rn == 0){
		if (p.str-- == 0 || (s = stringof(ss, p)) == nil)
			return 0;
		p.rn = s->n;
	}
	p.rn--;
	*pp = p;
	return 1;
}

int
_posinc(Stringset *ss, Position *pp)
{
	String *s;
	Position p;

	p = *pp;
	while ((s = stringof(ss, p)) && p.rn == s->n){
		p.str++;
		p.rn = 0;
	}
	if (s == nil)
		return 0;
	p.rn++;
	*pp = p;
	return 1;
}

int
_stringfindrune(Stringset *ss, Position *pp, Rune r)
{
	String *s;
	Position pos;

	s = nil;
	pos = *pp;
	for(;;){
		if (s){
			if (pos.rn < s->n){
				if (s->r[pos.rn] == r)
					break;
				else
					pos.rn++;
			}else{
				pos.str++;
				pos.rn = 0;
				s = nil;
			}
		}else if ((s = stringof(ss, pos)) == nil)
			return 0;
	}
	*pp = pos;
	return 1;
}

int
_stringfindrrune(Stringset *ss, Position *pp, Rune r)
{
	String *s;
	Position pos;

	s = nil;
	pos = *pp;
	for(;;){
		if (pos.rn < 0){
			if (--pos.str < 0)
				return 0;
			else{
				if (s = stringof(ss, pos)){
					pos.rn = s->n - 1;
				}else
					return 0;
			}
		} else {
			if (s){
				if (s->r[pos.rn] == r)
					break;
				else
					pos.rn--;
			}else if ((s = stringof(ss, pos)) == nil)
				return 0;
		}
	}
	*pp = pos;
	return 1;
}

void
_stringspace(Stringset *ss, int n)
{
	if (n <= ss->nalloc)
		return;
	debugprint(DBGSTRING, "stringspace: %d\n", n);
	ss->string = ctlrealloc(ss->string, n * sizeof(String*));
	memset(ss->string+ss->nalloc, 0, (n-ss->nalloc)*sizeof(String*));
	ss->nalloc = n;
}

void
_posadjust(Undo* c, Position *p)
{
	/* BEWARE: _posadjust is called AFTER the operation
	 * was inverted.  Therefore, the adjustment for a delete
	 * operation is found under the Insert case below
	 */
	debugprint(DBGPOSITION, "posperform: %d/%d ", p->str, p->rn);
	switch(c->op){
	case Delete:
		if(p->str >= c->p.str)
			p->str++;
		debugprint(DBGPOSITION, "insert %d: %d/%d\n",
			c->p.str, p->str, p->rn);
		break;
	case Insert:
		if (p->str == c->p.str)
			p->rn = 0;
		if(p->str > c->p.str)
			p->str--;
		debugprint(DBGPOSITION, "delete %d: %d/%d\n",
			c->p.str, p->str, p->rn);
		break;
	case Join:
		if(p->str > c->p.str)
			p->str++;
		else if (p->str == c->p.str && p->rn >= c->p.rn){
			p->rn -= c->p.rn;
			p->str++;
		}
		debugprint(DBGPOSITION, "split at %d/%d: %d/%d\n",
			c->p.str, c->p.rn, p->str, p->rn);
		break;
	case Split:
		if (p->str == c->p.str+1)
			p->rn += c->p.rn;
		if(p->str > c->p.str)
			p->str--;
		debugprint(DBGPOSITION, "join at %d/%d: %d/%d\n",
			c->p.str, c->p.rn, p->str, p->rn);
		break;
	}
}

void
perform(Stringset *ss, Undo* u)
{
	String *s;
	Position p;

	switch(u->op){
	case Insert:
		debugprint(DBGPERFORM, "insert: %d runes, context %s at line %d\n",
			u->s->n, u->s->context, u->p.str);
		if (ss->c)
			layoutprep((Edit*)ss->c, u->p, u->p);
		_stringspace(ss, ss->nstring + 1);
		memmove(ss->string + u->p.str + 1, ss->string + u->p.str, (ss->nstring - u->p.str)*sizeof(String*));
		ss->nstring++;
		ss->string[u->p.str] = u->s;
		if (u->s){
			u->s->ref++;
			debugprint(DBGSTRING, "perform: 0x%p ref++ → %d\n", u->s, u->s->ref);
		}
		break;
	case Delete:
		debugprint(DBGPERFORM, "delete: %d runes, context %s at line %d\n",
			u->s->n, u->s->context, u->p.str);
		p = u->p;
		p.str++;
		p.rn = 0;
		if (ss->c)
			layoutprep((Edit*)ss->c, u->p, p);
		freestring(ss->string[u->p.str]);
		ss->nstring--;
		memmove(ss->string + u->p.str, ss->string + u->p.str + 1, (ss->nstring - u->p.str)*sizeof(String*));
		break;
	case Split:
		debugprint(DBGPERFORM, "split: at position %d/%d\n",
			u->p.str, u->p.rn);
		p = u->p;
		p.str++;
		p.rn = 0;
		if (ss->c)
			layoutprep((Edit*)ss->c, u->p, p);
		_stringspace(ss, ss->nstring + 1);
		memmove(ss->string + u->p.str + 1, ss->string + u->p.str, (ss->nstring - u->p.str)*sizeof(String*));
		ss->nstring++;
		ss->string[u->p.str] = s = stringcow(ss->string[u->p.str]);
		ss->string[u->p.str+1] = _allocstring(s->context, s->r + u->p.rn, s->n - u->p.rn);
		s->n = u->p.rn;
		break;
	case Join:
		debugprint(DBGPERFORM, "join: strings %d and %d\n",
			u->p.str, u->p.str+1);
		p = u->p;
		p.str += 2;
		p.rn = 0;
		if (ss->c)
			layoutprep((Edit*)ss->c, u->p, p);
		ss->string[u->p.str] = s = stringcow(ss->string[u->p.str]);
		s->r = ctlrealloc(s->r, (s->n + ss->string[u->p.str+1]->n) * sizeof(Rune));
		memmove(s->r + s->n, ss->string[u->p.str+1]->r, ss->string[u->p.str+1]->n * sizeof(Rune));
		s->n += ss->string[u->p.str+1]->n;
		freestring(ss->string[u->p.str+1]);
		ss->nstring--;
		memmove(ss->string + u->p.str+1, ss->string + u->p.str + 2, (ss->nstring - u->p.str-1)*sizeof(String*));
		break;
	}
	u->op ^= 1;
	if (ss->c){
		adjustselections((Edit*)ss->c, u);
		adjustlines((Edit*)ss->c, u);
	}
}

void
split(Stringset *ss, Position p)
{
	Undo *u;

	u = _newundo(ss);
	u->op = Split;
	u->p = p;
	perform(ss, u);
}

void
join(Stringset *ss, Position p)
{
	Undo *u;

	u = _newundo(ss);
	u->op = Join;
	u->p = p;
	perform(ss, u);
}

void
insert(Stringset *ss, int n, String *s)
{
	Undo *u;

	u = _newundo(ss);
	u->op = Insert;
	u->p.str = n;
	u->p.rn = 0;
	u->s = s;
	if (u->s){
		u->s->ref++;
		debugprint(DBGSTRING, "insert: 0x%p ref++ → %d\n", u->s, u->s->ref);
	}
	perform(ss, u);
}

void
delete(Stringset *ss, int n)
{
	Undo *u;

	u = _newundo(ss);
	u->op = Delete;
	u->p.str = n;
	u->p.rn = 0;
	u->s = stringof(ss, u->p);
	if (u->s){
		u->s->ref++;
		debugprint(DBGSTRING, "delete: 0x%p ref++ → %d\n", u->s, u->s->ref);
	}
	perform(ss, u);
}

void
undobegingroup(Stringset *ss)
{
	if (ss->undogrouplevel++ == 0)
		ss->undobegin = ss->undostate;
}

void
undoendgroup(Stringset *ss)
{
	Undo *u;

	if (ss->undogrouplevel <= 0)
		ctlerror("undoendgroup: unbegun");
	if (--ss->undogrouplevel > 0)
		return;
	for (u = ss->undobegin->next; u != ss->undostate; u = u->next){
		if (u == nil)
			ctlerror("undoendgroup: at end of undo list");
		u->flags |= Nextalso;
	}
	ss->undobegin = nil;
}


void
undo(Stringset *ss)
{
	if (ss->undogrouplevel)
		ctlerror("undo: group undo in progress");
	if (ss->undostate == &ss->undohome){
		debugprint(DBGPERFORM, "undo: nothing to undo\n");
		return;	/* nothing to undo */
	}
	do {
		assert(ss->undostate != &ss->undohome);
		perform(ss, ss->undostate);
		ss->undostate = ss->undostate->prev;
	} while(ss->undostate->flags & Nextalso);
}

void
redo(Stringset *ss)
{
	if (ss->undogrouplevel)
		ctlerror("redo: group undo in progress");
	if (ss->undostate->next == nil)
		return;	/* nothing to redo */
	do {
		assert(ss->undostate != nil);
		ss->undostate = ss->undostate->next;
		perform(ss, ss->undostate);
	} while(ss->undostate->flags & Nextalso);
}

void
cut(Stringset *ss, Position p1, Position p2)
{
	String *s;
	int n;

	if (_posequal(p1, p2))
		return;
	assert(_posbefore(p1, p2));
	if (debug & DBGCUT)
		dumpstrings(ss);

	undobegingroup(ss);
	// start at the end to avoid compromising positions by changes
	s = stringof(ss, p2);
	if (s && p2.rn > 0 && p2.rn < s->n){
		split(ss, p2);
	}else if (p2.rn == 0)
		p2.str--;
	for (n = p2.str; n > p1.str; n--)
		delete(ss, n);
	s = stringof(ss, p1);
	if (s && p1.rn > 0 && p1.rn < s->n){
		split(ss, p1);
		delete(ss, p1.str+1);
	}else if (p1.rn == 0)
		delete(ss, p1.str);
	undoendgroup(ss);
	if (debug & DBGCUT)
		dumpstrings(ss);
}

void
snarf(Stringset *ss, Position p1, Position p2)
{
	String *s;
	int i;

	if (_posequal(p1, p2))
		return;
	assert(_posbefore(p1, p2));
	snarfclear(ss);

	debugprint(DBGSTRING, "snarf: %d/%d — %d/%d\n", p1.str, p1.rn, p2.str, p2.rn);

	undobegingroup(ss);
	if (ss->undostate != &ss->undohome)
		ss->undobegin = ss->undobegin->prev;	// combine with previous undo

	s = stringof(ss, p1);
	if (s && p1.rn > 0 && p1.rn < s->n){
		debugprint(DBGSTRING, "snarf: split at p1: %d/%d — %d/%d\n",
			p1.str, p1.rn, p2.str, p2.rn);
		split(ss, p1);
		p1.str++;
		_posadjust(ss->undostate, &p2);
		debugprint(DBGSTRING, "snarf: after split at p1: %d/%d — %d/%d\n",
			p1.str, p1.rn, p2.str, p2.rn);
	}else if (p1.rn == s->n)
		p1.str++;

	s = stringof(ss, p2);
	if (s && p2.rn > 0 && p2.rn < s->n)
		split(ss, p2);
	else if (p2.rn == 0)
		p2.str--;

	ss->nsnarf = p2.str - p1.str + 1;
	ss->snarf = ctlrealloc(ss->snarf, ss->nsnarf*sizeof(String*));
	for (i = p1.str; i <= p2.str; i++){
		debugprint(DBGSTRING, "snarf: copy string: %d 0x%p\n",
			i, ss->string[i]);
		ss->snarf[i-p1.str] = ss->string[i];
		if (ss->string[i]) ss->string[i]->ref++;
	}
	undoendgroup(ss);
	if (ss->c)
		chanprint(ss->c->event, "%q: snarf %d", ss->c->name, ss->nsnarf);
}

void
_stringadd(Stringset *ss, String *s, int n)
{
	int i;

	debugprint(DBGUSER, "addstring %d\n", n);
	if (n >= ss->nstring){
		debugprint(DBGUSER, "addstring: allocating string %d\n", n);
		ss->string = ctlrealloc(ss->string, (n+1)*sizeof(String*));
		for (i = ss->nstring; i < n; i++)
			ss->string[i] = nil;
		ss->nstring = n+1;
	}else if (ss->string[n]){
		debugprint(DBGUSER, "removing string %d\n", n);
		freestring(ss->string[n]);
		ss->string[n] = nil;
	}
	ss->string[n] = s;
	s->ref++;
}

void
paste(Stringset *ss, Position pos, String **st, int ns)
{
	String *s;
	int i;

	undobegingroup(ss);
	s = stringof(ss, pos);
	if (s == nil || pos.rn < 0 || pos.rn > s->n){
		if (ss->c)
			ctlerror("%q: paste in illegal position: %d/%d", ss->c->name, pos.str, pos.rn);
		else
			ctlerror("paste in illegal position: %d/%d", pos.str, pos.rn);
	}
	if (pos.rn){
		if (pos.rn < s->n){
			debugprint(DBGPASTE, "paste: middle of string\n");
			split(ss, pos);
		}else
			debugprint(DBGPASTE, "paste: at end of string\n");
		pos.str++;
	}
	for (i = 0; i < ns; i++)
		insert(ss, pos.str+i, st[i]);
	undoendgroup(ss);
}

Position
insertrunes(Stringset *ss, Position p, Rune *rp, int nr)
{
	String *s;

	s = stringof(ss, p);
	if (s == nil){
		if (ss->c)
			ctlerror("%q: insert into non string at position %d/%d", ss->c->name, p.str, p.rn);
		else
			ctlerror("insert into non string at position %d/%d", p.str, p.rn);
	}
	s = _allocstring(s->context, rp, nr);
	paste(ss, p, &s, 1);
	freestring(s);
	p.str++;
	p.rn = 0;
	return p;
}

Position
_addonerune(Stringset *ss, Position pos, Rune rune)
{
	String *s;

	if ((s = stringof(ss, pos))	// pos points into existing string
		&& pos.rn == s->n	// at its end
		&& ss->undostate != &ss->undohome	// there's something to undo
		&& ss->undostate->op == Delete	// current undo is a delete
		&& ss->undostate->p.str == pos.str	// refers to current string
		&& ss->undostate->s == s	// and (still) uses the same copy of it
		&& s->ref == 2	// and these strings are not used elsewhere
	){
		/* Just add rune to the end of current String, which automatically
		 * adds it to current Undo as well
		 */
		debugprint(DBGCUT|DBGPASTE, "addonerune: optimized case\n");
		if (ss->c)
			layoutprep((Edit*)ss->c, pos, pos);
		s->r = ctlrealloc(s->r, (s->n+1)*sizeof(Rune));
		s->r[s->n++] = rune;
		pos.rn = s->n;
		return pos;
	}
	debugprint(DBGCUT|DBGPASTE, "addonerune: normal case\n");
	return insertrunes(ss, pos, &rune, 1);
}

Position
_delonerune(Stringset *ss, Position pos)
{
	String *s;
	Position p2;

	p2 = pos;
	if ((s = stringof(ss, pos))	// points into existing string
		&& s->n > 1	// it's got more than one character
		&& pos.rn == s->n	// we're at its end
		&& ss->undostate != &ss->undohome	// there's something to undo
		&& ss->undostate->op == Delete	// current undo is a delete
		&& ss->undostate->p.str == pos.str	// refers to current string
		&& ss->undostate->s == s	// and (still) uses the same copy of it
		&& s->ref == 2	// and these strings are not used elsewhere
	){
		/* Just delete a rune from the end of current String, which automatically
		 * incorporates it into current Undo
		 */
		debugprint(DBGCUT|DBGPASTE, "delonerune: optimized case\n");
		pos.rn--;
		if (ss->c)
			layoutprep((Edit*)ss->c, pos, p2);
		s->n--;
		return pos;
	}
	debugprint(DBGCUT|DBGPASTE, "delonerune: normal case\n");
	// backup one character
	if (pos.rn)
		pos.rn--;
	else for(;;) {
		String *s;

		if (pos.str == 0 ||  ss->string[pos.str-1] == nil){
			debugprint(DBGCUT|DBGPASTE, "delonerune: beginning of file\n");
			return pos;
		}
		s = ss->string[pos.str-1];
		if (s->n){
			pos.str--;
			pos.rn = s->n-1;
			p2.str = pos.str;
			p2.rn = s->n;
			break;
		}
	}
	cut(ss, pos, p2);
	return pos;
}

void
freestring(String *s)
{
	debugprint(DBGSTRING, "freestring: 0x%p ref %d [%d]", s, s?s->ref:0, s?s->n:0);
	if (s && (debug & DBGSTRING)){
		int i;
		for (i = 0; i < s->n; i++)fprint(2, "%C", s->r[i]);
		fprint(2, "\n");
	}
	if (s == nil || --s->ref > 0)
		return;
	if (s->ref < 0)
		abort();
	free(s->context);
	free(s->r);
	free(s);
}

String *
_allocstring(char *c, Rune *r, int n)
{
	String *s;

	s = ctlmalloc(sizeof(String));
	s->ref = 1;
	s->context = strdup(c);
	if (s->n = n){
		s->r = ctlmalloc(n*sizeof(Rune));
		memmove(s->r, r, n*sizeof(Rune));
	}else
		s->r = nil;
	debugprint(DBGSTRING, "_allocstring: 0x%p %s [%d]", s, c, n);
	if (debug & DBGSTRING){
		int i;
		for (i = 0; i < n; i++)fprint(2, "%C", r[i]);
		fprint(2, "\n");
	}
	return s;
}

String *
allocstring(char *context, char *p)
{
	String *s;

	if (_contextnamed(context) == nil)
		ctlerror("allocstring: no such context %q", context);
	s = ctlmalloc(sizeof(String));
	s->ref = 1;
	s->context = strdup(context);
	s->r = _ctlrunestr(p);
	s->n = runestrlen(s->r);
	debugprint(DBGSTRING, "_allocstring: 0x%p %s [%d]", s, context, s->n);
	if (debug & DBGSTRING){
		int i;
		for (i = 0; i < s->n; i++) fprint(2, "%C", s->r[i]);
		fprint(2, "\n");
	}
	return s;
}

static String*
stringcow(String *s)	// cow: copy on write
{
	if (s->ref == 1)
		return s;
	--s->ref;
	assert(s->ref > 0);
	return _allocstring(s->context, s->r, s->n);
}

void
emptyundo(Stringset *ss)
{
	Undo *u;

	if (ss->undobegin)
		ctlerror("emptyundo: group undo in progress");
	while ((u = ss->undostate) != &ss->undohome){
		freestring(u->s);
		ss->undostate = u->prev;
		ss->undostate->next = u->next;
		free(u);
	}
}

static Undo *
_newundo(Stringset *ss)
{
	Undo *u;

	if (ss->undostate->next == nil){
		ss->undostate->next = u = ctlmalloc(sizeof(Undo));
		u->prev = ss->undostate;
		u->next = nil;
		u->op = Noop;
		u->flags = 0;
		u->s = nil;
	}else{
		for (u = ss->undostate->next; u && u->op != Noop; u = u->next){
			freestring(u->s);
			u->s = nil;
			u->op = Noop;
			u->flags = 0;
		}
	}
	ss->undostate = ss->undostate->next;
	return ss->undostate;
}

Context *
_newcontext(char *name)
{
	Context *c;

	if (_contextnamed(name))
		ctlerror("newcontext: context %s exists", name);
	c = ctlmalloc(sizeof(Context));
	c->next = context;
	context = c;
	c->name = strdup(name);
	c->f = nil;
	c->wordbreak = 0;
	c->tabs[0] = -1;
	
	return c;
}
