#include <u.h>
#include <libc.h>
#include <bio.h>
#include <draw.h>
#include <html.h>
#include "dat.h"

/*
 * Search for something else to focus on.
 */
typedef struct Search Search;
struct Search
{
	int type;
	int region;
	Rune *r;
	int havebest;
	Focus best;
	Focus found;
	int (*match)(Search*, Item*);
	int (*notematch)(Search*, Focus*, Item*);
};

static void
search(Focus *f, Search *s)
{
	int i;
	Item *x;
	
	s->region = 0;
	for(i=0; i<nflatitem; i++){
		x = flatitem[i];
		if(x == f->ifirst)
			s->region = 1;
		if(s->match(s, x))
			if(s->notematch(s, f, x))
				break;
		if(x == f->ilast)
			s->region = 2;
	}
}

static int
notematchfirst(Search *s, Focus *f, Item *x)
{
	USED(f);
	USED(x);

	if(!s->havebest){
		s->havebest = 1;
		s->best = s->found;
		return 1;
	}
	return 0;
}

static int
notematchinner(Search *s, Focus *f, Item *x)
{
	USED(f);
	USED(x);
	
	if(s->region == 1 && !s->havebest){
		s->havebest = 1;
		s->best = s->found;
		return 1;
	}
	return 0;
}

static int
listencloses(Item *x, Item *xend, Item *y)
{
	Tablecell *c;
	
	for(; x != xend; x=x->next){
		if(x == y)
			return 1;
		switch(x->tag){
		case Ifloattag:
			if(listencloses(((Ifloat*)x)->item, nil, y))
				return 1;
			break;
		case Itabletag:
			for(c=((Itable*)x)->table->cells; c; c=c->next)
				if(listencloses(c->content, nil, y))
					return 1;
			break;
		}
	}
	return 0;
}

static int
encloses(Item *x, Item *y)
{
	return listencloses(x, x->next, y);
}

static int
notematchouter(Search *s, Focus *f, Item *x)
{
	USED(x);

//fprint(2, "region=%d enclose=%d\n", s->region, encloses(s->found.ifirst, f->ifirst));
	if(s->region == 0 && encloses(s->found.ifirst, f->ifirst)){
		s->havebest = 1;
		s->best = s->found;
		return 0;
	}
	return s->region > 0;
}

static int
notematchprev(Search *s, Focus *f, Item *x)
{
	USED(f);
	USED(x);
	
	if(s->region == 0){
		s->havebest = 1;
		s->best = s->found;
		return 0;
	}
	return s->region > 0;
}

static int
notematchnext(Search *s, Focus *f, Item *x)
{
	USED(f);
	USED(x);
	
	if(s->region == 2 && !s->havebest){
		s->havebest = 1;
		s->best = s->found;
		return 1;
	}
	return 0;
}

/*
static int
matchany(Search *s, Item *x)
{
	USED(s);
	USED(x);
	return 1;
}
*/

static int
matchtext(Search *s, Item *x)
{
	Itext *itext;
	
	if(x->tag != Itexttag)
		return 0;
	itext = (Itext*)x;
	if(runestrstr(itext->s, s->r) == nil)
		return 0;
	focusitem(&s->found, x);
	return 1;
}

static int
matchtable(Search *s, Item *x)
{
	Table *t;
	Tablecell *c;
	
	if(!(x->state&IFtable))
		return 0;
	
	for(t=doc->docinfo->tables; t; t=t->next){
		for(c=t->cells; c; c=c->next){
			if(c->content == x){
				focustable(&s->found, t);
				return 1;
			}
			if(c->content)
				break;
		}
	}
	return 0;
}

static int
matchtablecell(Search *s, Item *x)
{
	Table *t;
	Tablecell *c;
	
	if(!(x->state&IFcell))
		return 0;
	
	for(t=doc->docinfo->tables; t; t=t->next){
		for(c=t->cells; c; c=c->next){
			if(c->content == x){
				focustablecell(&s->found, c);
				return 1;
			}
		}
	}
	return 0;
}

static int
matchtablerow(Search *s, Item *x)
{
	int i;
	Table *t;
	Tablecell *c;
	
	if(!(x->state&IFrow))
		return 0;
	
	for(t=doc->docinfo->tables; t; t=t->next){
		for(i=0; i<t->nrow; i++){
			for(c=t->rows[i].cells; c; c=c->nextinrow){
				if(c->content == x){
					focustablerow(&s->found, &t->rows[i]);
					return 1;
				}
				if(c->content)
					break;
			}
		}
	}
	return 0;
}

static int
matchform(Search *s, Item *x)
{
	Iformfield *iff;
	
	if(x->tag != Iformfieldtag)
		return 0;
	iff = (Iformfield*)x;
	focusform(&s->found, iff->formfield->form);
	return 1;
}

static int
matchformfield(Search *s, Item *x, int ftype)
{
	Iformfield *iff;
	
	if(x->tag != Iformfieldtag)
		return 0;
	iff = (Iformfield*)x;
	if(iff->formfield->ftype != ftype)
		return 0;
	/*
	 * XXX should use value instead?
	 */
	if(s->r && runestrcmp(iff->formfield->name, s->r) != 0)
		return 0;
	focusformfield(&s->found, iff->formfield);
	return 1;
}

static int
matchcheckbox(Search *s, Item *x)
{
	return matchformfield(s, x, Fcheckbox);
}

static int
matchtextbox(Search *s, Item *x)
{
	return matchformfield(s, x, Ftext);
}

static int
matchpassword(Search *s, Item *x)
{
	return matchformfield(s, x, Fpassword);
}

static int
matchradiobutton(Search *s, Item *x)
{
	return matchformfield(s, x, Fradio);
}

static int
matchselect(Search *s, Item *x)
{
	return matchformfield(s, x, Fselect);
}

static int
matchtextarea(Search *s, Item *x)
{
	return matchformfield(s, x, Ftextarea);
}

static int
matchpage(Search *s, Item *x)
{
	USED(s);
	USED(x);
	focuspage(&s->found);
	return 1;
}

int
find(int where, int thing, char *str)
{
	Search s;
	
	memset(&s, 0, sizeof s);
	switch(where){
	default:
	case Wnone:
		s.notematch = notematchfirst;
		break;
	case Winner:
		s.notematch = notematchinner;
		break;
	case Wouter:
		s.notematch = notematchouter;
		break;
	case Wprev:
		s.notematch = notematchprev;
		break;
	case Wnext:
		s.notematch = notematchnext;
		break;
	}
	
	switch(thing){
	default:
	case Tnone:
		s.match = matchtext;
		break;
	case Tpage:
		s.match = matchpage;
		break;
	case Ttext:
		s.match = matchtext;
		break;
	case Ttable:
		s.match = matchtable;
		break;
	case Tcell:
		s.match = matchtablecell;
		break;
	case Trow:
		s.match = matchtablerow;
		break;
	case Tform:
		s.match = matchform;
		break;
	case Tcheckbox:
		s.match = matchcheckbox;
		break;
	case Ttextbox:
		s.match = matchtextbox;
		break;
	case Ttextarea:
		s.match = matchtextarea;
		break;
	case Tpassword:
		s.match = matchpassword;
		break;
	case Tradiobutton:
		s.match = matchradiobutton;
		break;
	case Tselect:
		s.match = matchselect;
		break;
	}
	
	if(str)
		s.r = toStr((uchar*)str, strlen(str), UTF_8);
	search(&focus, &s);
	if(str)
		free(s.r);
	if(!s.havebest)
		return 0;
	focus = s.best;
	return 1;
}

