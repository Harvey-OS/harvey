#include <u.h>
#include <libc.h>
#include <bio.h>
#include <draw.h>
#include <regexp.h>
#include <html.h>
#include <ctype.h>
#include "dat.h"

char urlexpr[] = "^(https?|ftp|file|gopher|mailto|news|nntp|telnet|wais|prospero)://([a-zA-Z0-9_@\\-]+([.:][a-zA-Z0-9_@\\-]+)*)";
Reprog	*urlprog;

int aflag = 0;
int defcharset = UTF_8;
int width = 70;

int inword = 0;
int col = 0;
int wordi = 0;

char*
runetobyte(Rune *r, int n)
{
	char *s;

	if(n == 0)
		return emalloc(1);
	s = smprint("%.*S", n, r);
	if(s == nil)
		error("malloc failed");
	return s;
}

int
closingpunct(int c)
{
	return strchr(".,:;'\")]}>!?", c) != nil;
}

void
emitword(Bytes *b, Rune *r, int nr)
{
	char *s;
	int space;

	if(nr == 0)
		return;
	s = smprint("%.*S", nr, r);
	space = (b->n>0) && !isspace(b->b[b->n-1]) && !closingpunct(r[0]);
	if(col>0 && col+space+nr > width){
		growbytes(b, "\n", 1);
		space = 0;
		col = 0;
	}
	if(space && col>0){
		growbytes(b, " ", 1);
		col++;
	}
	growbytes(b, s, strlen(s));
	col += nr;
	free(s);
	inword = 0;
}

void
renderrunes(Bytes *b, Rune *r)
{
	int i, n;

	n = runestrlen(r);
	for(i=0; i<n; i++){
		switch(r[i]){
		case '\n':
			if(inword)
				emitword(b, r+wordi, i-wordi);
			col = 0;
			if(b->n == 0)
				break;	/* don't start with blank lines */
			if(b->n<2 || b->b[b->n-1]!='\n' || b->b[b->n-2]!='\n')
				growbytes(b, "\n", 1);
			break;
		case ' ':
			if(inword)
				emitword(b, r+wordi, i-wordi);
			break;
		default:
			if(!inword)
				wordi = i;
			inword = 1;
			break;
		}
	}
	if(inword)
		emitword(b, r+wordi, i-wordi);
}

void
renderbytes(Bytes *b, char *fmt, ...)
{
	Rune *r;
	va_list arg;

	va_start(arg, fmt);
	r = runevsmprint(fmt, arg);
	va_end(arg);
	renderrunes(b, r);
	free(r);
}

char*
baseurl(char *url)
{
	char *base, *slash;
	Resub rs[10];

	if(url == nil)
		return nil;
	if(urlprog == nil){
		urlprog = regcomp(urlexpr);
		if(urlprog == nil)
			error("can't compile URL regexp");
	}
	memset(rs, 0, sizeof rs);
	if(regexec(urlprog, url, rs, nelem(rs)) == 0)
		return nil;
	base = estrdup(url);
	slash = strrchr(base, '/');
	if(slash!=nil && slash>=&base[rs[0].ep-rs[0].sp])
		*slash = '\0';
	else
		base[rs[0].ep-rs[0].sp] = '\0';
	return base;
}

char*
fullurl(URLwin *u, Rune *rhref)
{
	char *base, *href, *hrefbase;
	char *result;

	if(rhref == nil)
		return estrdup("NULL URL");
	href = runetobyte(rhref, runestrlen(rhref));
	hrefbase = baseurl(href);
	result = nil;
	if(hrefbase==nil && (base = baseurl(u->url))!=nil){
		result = estrdup(base);
		if(base[strlen(base)-1]!='/' && (href==nil || href[0]!='/'))
			result = eappend(result, "/", "");
		free(base);
	}
	if(href){
		if(result)
			result = eappend(result, "", href);
		else
			result = estrdup(href);
	}
	free(hrefbase);
	if(result == nil)
		return estrdup("***unknown***");
	return result;
}

static Rune rempty[1];

void
render(URLwin *u, Bytes *t, Item *items, Item *last, int curanchor)
{
	int r;
	Item *il;
	Itext *it;
	Ifloat *ifl;
	Ispacer *is;
	Itable *ita;
	Iimage *im;
	Anchor *a;
	Formfield *ff;
	Table *tab;
	Tablecell *cell, *cnext;
	char *href;

	inword = 0;
	col = 0;
	wordi = 0;

	for(il=items; il!=last; il=il->next){
		if(il->state & IFbrk)
			renderbytes(t, "\n");
		if(il->state & IFbrksp)
			renderbytes(t, "\n");

		switch(il->tag){
		case Itexttag:
			it = (Itext*)il;
			renderrunes(t, it->s);
			break;
		case Iruletag:
			if(t->n>0 && t->b[t->n-1]!='\n')
				renderbytes(t, "\n");
			renderbytes(t, "=======\n");
			break;
		case Iimagetag:
			if(!aflag)
				break;
			im = (Iimage*)il;
			if(im->imsrc){
				href = fullurl(u, im->imsrc);
				renderbytes(t, "[image %s]", href);
				free(href);
			}
			break;
		case Iformfieldtag:
			ff = ((Iformfield*)il)->formfield;
			renderbytes(t, "[%F %S=%S]", ff->ftype, 
				ff->name, ff->value ? ff->value : rempty);
			break;
		case Itabletag:
			ita = (Itable*)il;
			tab = ita->table;
			cnext = tab->cells;
			for(r=0; r<tab->nrow; r++){
				for(cell=tab->rows[r].cells; cell; cell=cell->nextinrow){
					if(cell == cnext){
						cnext = cell->next;
						if(cell != tab->rows[r].cells)
							renderbytes(t, "	");
						render(u, t, cell->content, nil, curanchor);
						if(t->n>0 && t->b[t->n-1] == '\n')
							t->n--;
					}
				}
				if(t->n>0 && t->b[t->n-1]!='\n')
					renderbytes(t, "\n");
			}
		/*
			for(cell=tab->cells; cell!=nil; cell=cell->next){
				render(u, t, cell->content, nil, curanchor);
			}
		*/
			if(t->n>0 && t->b[t->n-1]!='\n')
				renderbytes(t, "\n");
			break;
		case Ifloattag:
			ifl = (Ifloat*)il;
			render(u, t, ifl->item, nil, curanchor);
			break;
		case Ispacertag:
			is = (Ispacer*)il;
			if(is->spkind != ISPnull)
				renderbytes(t, " ");
			break;
		default:
			error("unknown item tag %d\n", il->tag);
		}
		if(il->anchorid != 0 && il->anchorid!=curanchor){
			for(a=u->docinfo->anchors; a!=nil; a=a->next)
				if(aflag && a->index == il->anchorid){
					href = fullurl(u, a->href);
					renderbytes(t, "[%s]", href);
					free(href);
					break;
				}
			curanchor = il->anchorid;
		}
	}
	if(t->n>0 && t->b[t->n-1]!='\n')
		renderbytes(t, "\n");
}

void
rerender(URLwin *u)
{
	Bytes *t;

	t = emalloc(sizeof(Bytes));

	render(u, t, u->items, nil, 0);

	if(t->n)
		write(1, (char*)t->b, t->n);
	free(t->b);
	free(t);
}

/*
 * Somewhat of a hack.  Not a full parse, just looks for strings in the beginning
 * of the document (cistrstr only looks at first somewhat bytes).
 */
int
charset(char *s)
{
	char *meta, *emeta, *charset;

	if(defcharset == 0)
		defcharset = ISO_8859_1;
	meta = cistrstr(s, "<meta");
	if(meta == nil)
		return defcharset;
	for(emeta=meta; *emeta!='>' && *emeta!='\0'; emeta++)
		;
	charset = cistrstr(s, "charset=");
	if(charset == nil)
		return defcharset;
	charset += 8;
	if(*charset == '"')
		charset++;
	if(cistrncmp(charset, "utf-8", 5) || cistrncmp(charset, "utf8", 4))
		return UTF_8;
	return defcharset;
}

void
rendertext(URLwin *u, Bytes *b)
{
	Rune *rurl;

	rurl = toStr((uchar*)u->url, strlen(u->url), ISO_8859_1);
	u->items = parsehtml(b->b, b->n, rurl, u->type, charset((char*)b->b), &u->docinfo);
//	free(rurl);

	rerender(u);
}

void
freeurlwin(URLwin *u)
{
	freeitems(u->items);
	u->items = nil;
	freedocinfo(u->docinfo);
	u->docinfo = nil;
	free(u);
}

static int
chainsto(Item *x, Item *y)
{
	for(; x; x=x->next)
		if(x == y)
			return 1;
	if(y == nil)
		return 1;
	return 0;
}

/*
 * Print selection.
 */
void
printfocus(Focus *focus)
{
	int i;
	Bytes *t;
	Tablecell *c;
	Item *x;
	
	t = emalloc(sizeof(Bytes));
	switch(focus->type){
	case FocusNone:
	case FocusStart:
	case FocusEnd:
		break;
	case FocusAnchor:
	case FocusForm:
	case FocusFormfield:
		i = flatindex(focus->ilast);
		if(i >= nflatitem-1 || chainsto(focus->ifirst, flatitem[i+1])){
			/* focus lines up with structure */
			render(doc, t, focus->ifirst, flatitem[i+1], 0);
			break;
		}
		/* focus doesn't line up with structure - clumsy hack */
		for(i=flatindex(focus->ifirst); i<nflatitem; ){
			x = flatitem[i];
			render(doc, t, x, x->next, 0);
			do
				i++;
			while(i<nflatitem && flatitem[i] != x->next);
		}
		break;
	case FocusItem:
	case FocusTable:
		render(doc, t, focus->ifirst, focus->ifirst->next, 0);
		break;
	case FocusPage:
	case FocusTablecell:
		render(doc, t, focus->ifirst, nil, 0);
		break;
	case FocusTablerow:
		for(c=focus->tablerow->cells; c; c=c->nextinrow){
			render(doc, t, c->content, nil, 0);
			if(t->n>0){
				if(t->b[t->n-1] == '\n')
					t->n--;
				renderbytes(t, "\t");
			}
		}
		break;
	default:
		renderbytes(t, "unknown focus");
	}
	if(t->n){
		if(t->b[t->n-1] != '\n')
			renderbytes(t, "\n");
		write(1, t->b, t->n);
	}
	free(t->b);
	free(t);
}

