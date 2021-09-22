#include <u.h>
#include <libc.h>
#include <thread.h>
#include <draw.h>
#include <mouse.h>
#include <keyboard.h>
#include <control.h>
#include <editcontrol.h>
#include "string.h"

static char *changename[] = {
	[Noop] = "Noop",
	[Insert] = "Insert",
	[Delete] = "Delete",
	[Split] = "Split",
	[Join] = "Join",
};

void
dumpundo(Stringset *ss)
{
	String *s;
	Undo *c;
	int j;
	int redo;

	fprint(2, "Undo/Redo dump:\n");
	redo = ss->undostate == &ss->undohome;
	for (c = ss->undohome.next; c; c = c->next){
		fprint(2, "Change 0x%p: %sdo ", c, redo?"re":"un");
		if (c == ss->undostate)
			redo++;
		fprint(2, "%s%s: %d/%d 0x%p", changename[c->op],
			(c->flags&Nextalso)?" nextalso":"", c->p.str, c->p.rn, c->s);
		if (s = c->s){
			fprint(2, " ref %d: %-8s [%d]", s->ref, s->context, s->n);
			for (j = 0; j < s->n; j++)
				fprint(2, "%C", s->r[j]);
		}
		fprint(2, "\n");
	}
}

void
dumpselections(Edit *e)
{
	Selection *s;

	fprint(2, "Selection dump:\n");
	for (s = e->sel; s < e->sel + nelem(e->sel); s++){
		fprint(2, "Selection %ld", s - e->sel);
		if (s->state & Dragging)
			fprint(2, ", dragging %s", (s->state&Drag1)?"right":"left");
		if (s->state & Set)
			fprint(2, ", set");
		if (s->state & Enabled)
			fprint(2, ", enabled");
		if (s->state & Ticked)
			fprint(2, ", ticked");
		if (s->state & (Set|Dragging))
			fprint(2, ", %d/%d 0x%p %P — %d/%d 0x%p %P",
				s->mpl.pos.str, s->mpl.pos.rn, s->mpl.f, s->mpl.p, s->mpr.pos.str, s->mpr.pos.rn, s->mpr.f, s->mpr.p);
		fprint(2, "\n");
	}
}

void
dumpstrings(Stringset *ss)
{
	String *s;
	int i, j;

	fprint(2, "String dump:\n");
	for (i = 0; i < ss->nstring; i++){
		if (s = ss->string[i]){
			fprint(2, "[%d] 0x%p: ref %d %-8s ", i, s, s->ref, s->context);
			for (j = 0; j < s->n; j++)
				fprint(2, "%C", s->r[j]);
			fprint(2, "\n");
		}else{
			fprint(2, "[%d]: empty\n", i);
		}
	}
	fprint(2, "Snarf buffer dump:\n");
	for (i = 0; i < ss->nsnarf; i++){
		if (s = ss->snarf[i]){
			fprint(2, "[%d] 0x%p: ref %d %-8s ", i, s, s->ref, s->context);
			for (j = 0; j < s->n; j++)
				fprint(2, "%C", s->r[j]);
			fprint(2, "\n");
		}else{
			fprint(2, "[%d]: empty\n", i);
		}
	}
}

void
dumplines(Edit *e)
{
	Line *l;
	Frag *f;
	String *s;
	int j;

	fprint(2, "Line/Frag dump:\n");
	for (l = e->l; l; l = l->next){
		fprint(2, "Line 0x%p: %R, %R\n", l, l->r, l->rplay);
		for (f = l->f; f; f = f->next){
			fprint(2, "	Frag 0x%p: %d/%d - %d/%d, 0x%p, %R [%d]",
				f, f->spos.str, f->spos.rn, f->epos.str, f->epos.rn, f->l, fragr(f),
				f->epos.rn-f->spos.rn);
			s = stringof(e->ss, f->spos);
			for (j = f->spos.rn; j < f->epos.rn; j++)
				fprint(2, "%C", s->r[j]);
			fprint(2, "\n");
		}
	}
}

int
debugprint(int mask, char *fmt, ...)
{
	int n;
	va_list arg;

	if ((debug & mask) == 0)
		return 0;
	va_start(arg, fmt);
	n = vfprint(2, fmt, arg);
	va_end(arg);
	return n;
}
