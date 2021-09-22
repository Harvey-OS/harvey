#include <u.h>
#include <libc.h>
#include <bio.h>
#include <draw.h>
#include <html.h>
#include "dat.h"

URLwin	*doc;
Item	**flatitem;
int		nflatitem;

/*
 * Prepare the flatitem[] array; add extra flags to mark tables, etc.
 */
static void
walkdoc(Item *i)
{
	int first, j;
	Item *x;
	Table *table;
	Tablecell *c;
	
	for(; i; i=i->next){
		if(nflatitem%128 == 0)
			flatitem = erealloc(flatitem, (nflatitem+129)*sizeof(flatitem[0]));
		flatitem[nflatitem++] = i;
		switch(i->tag){
		case Ifloattag:
			x = ((Ifloat*)i)->item;
			if(x){
				x->state |= IFfloat;
				walkdoc(x);
			}
			break;
		case Itabletag:
			first = 1;
			table = ((Itable*)i)->table;
			for(j=0; j<table->nrow; j++){
				for(c=table->rows[j].cells; c; c=c->nextinrow){
					x = c->content;
					if(x){
						x->state |= IFrow;
						break;
					}
				}
			}
			for(c=table->cells; c; c=c->next){
				x = c->content;
				if(x){
					x->state |= IFcell;
					if(first){
						first = 0;
						x->state |= IFtable;
					}
					walkdoc(x);
				}
			}
			break;
		}
	}
}

int
flatindex(Item *x)
{
	int i;
	
	for(i=0; i<nflatitem; i++)
		if(flatitem[i] == x)
			return i;
	if(x == nil)
		return nflatitem;
	return -1;
}

void
loadurl(char *url, char *post)
{
	Bytes *b;
	Rune *rurl;
	URLwin *u;
	
	if(doc){
		rurl = toStr((uchar*)url, strlen(url), UTF_8);
		url = fullurl(doc, rurl);
		free(rurl);
	}
	b = httpget(url, post);
	rurl = toStr((uchar*)url, strlen(url), UTF_8);
	u = emalloc(sizeof(URLwin));
	u->url = estrdup(url);
	u->type = TextHtml;
	u->items = parsehtml(b->b, b->n, rurl, u->type, charset((char*)b->b), &u->docinfo);
	doc = u;
	nflatitem = 0;
	flatitem = erealloc(flatitem, sizeof(flatitem[0]));
	walkdoc(u->items);
	flatitem[nflatitem] = nil;
	focuspage(&focus);
}

