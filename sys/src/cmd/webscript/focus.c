#include <u.h>
#include <libc.h>
#include <bio.h>
#include <draw.h>
#include <html.h>
#include "dat.h"

Focus	focus;

/*
 * The only real variable in the webscript language is the `focus,'
 * the currently selected item on the page.  After `load' or `submit',
 * the focus is the entire page, but `select' commands change it.
 * Libhtml doesn't really make our lives easier here -- there are
 * many different types of structure that might be the focus.
 * On the other hand, we don't have to parse HTML, and that's a plus.
 */

/*
 * Return rightmost item in tree/list rooted at i.
 */
static Item*
walklast(Item *i)
{
	Item *j;
	Item *last;
	Tablecell *c;
	
	last = nil;
	for(; i; i=i->next){
		last = i;
		switch(i->tag){
		case Ifloattag:
			j = ((Ifloat*)i)->item;
			if(j)
				last = walklast(j);
			break;
		case Itabletag:
			for(c=((Itable*)i)->table->cells; c; c=c->next)
				if(c->content)
					last = walklast(c->content);
			break;
		}
	}
	return last;			
}

void
focusitems(Focus *f, Item *i0, Item *i1)
{
	f->ifirst = i0;
	f->ilast = i1;
}

void
focusanchor(Focus *f, Anchor *a, Item *i)
{
	Item *last, *j;
	
	last = nil;
	for(j=i; j && j->anchorid == a->index; j=j->next)
		last = j;
	focusitems(f, i, last);
	f->type = FocusAnchor;
	f->anchor = a;
}

void
focusform(Focus *f, Form *form)
{
	int i;
	Item *ifirst, *ilast, *x;
	
	f->type = FocusForm;
	f->form = form;
	ifirst = nil;
	ilast = nil;
	for(i=0; i<nflatitem; i++){
		x = flatitem[i];
		if(x->tag == Iformfieldtag && ((Iformfield*)x)->formfield->form == form){
			if(ifirst == nil)
				ifirst = x;
			ilast = x;
		}
	}
	focusitems(f, ifirst, ilast);
}

void
focusformfield(Focus *f, Formfield *ff)
{
	int i;
	Item *ifirst, *ilast, *x;
	
	f->type = FocusFormfield;
	f->formfield = ff;
	ifirst = nil;
	ilast = nil;
	for(i=0; i<nflatitem; i++){
		x = flatitem[i];
		if(x->tag == Iformfieldtag && ((Iformfield*)x)->formfield == ff){
			if(ifirst == nil)
				ifirst = x;
			ilast = x;
		}
	}
	focusitems(f, ifirst, ilast);
}

void
focusitem(Focus *f, Item *i)
{
	f->type = FocusItem;
	focusitems(f, i, i);
}

void
focustable(Focus *f, Table *t)
{
	int i;
	Tablecell *c;
	Item *ifirst, *ilast, *x;
	
	f->type = FocusTable;
	f->table = t;
	ifirst = nil;
	for(i=0; i<nflatitem; i++){
		x = flatitem[i];
		if(x->tag==Itabletag && ((Itable*)x)->table == t){
			ifirst = x;
			break;
		}
	}
	if(i == nflatitem)
		fprint(2, "warning: cannot find table\n");
	ilast = nil;
	for(c=t->cells; c; c=c->next){
		if(ifirst == nil && c->content)
			ifirst = c->content;
		if(c->content)
			ilast = walklast(c->content);
	}
	focusitems(f, ifirst, ilast);
}

void
focustablecell(Focus *f, Tablecell *c)
{
	f->type = FocusTablecell;
	f->tablecell = c;
	focusitems(f, c->content, walklast(c->content));
}

void
focustablerow(Focus *f,Tablerow *r)
{
	Item *ifirst, *ilast;
	Tablecell *c;
	
	f->type = FocusTablerow;
	f->tablerow = r;
	ifirst = nil;
	ilast = nil;
	for(c=r->cells; c; c=c->nextinrow){
		if(ifirst == nil && c->content)
			ifirst = c->content;
		if(c->content)
			ilast = walklast(c->content);
	}
	focusitems(f, ifirst, ilast);
}

void
focuspage(Focus *f)
{
	f->type = FocusPage;
	if(nflatitem == 0)
		focusitems(f, nil, nil);
	else
		focusitems(f, flatitem[0], flatitem[nflatitem-1]);
}

void
focusstart(Focus *f)
{
	f->type = FocusStart;
	focusitems(f, nil, nil);
}

void
focusend(Focus *f)
{
	f->type = FocusEnd;
	focusitems(f, nil, nil);
}

