#include <u.h>
#include <libc.h>
#include <libg.h>
#include <frame.h>
#include "dat.h"
#include "fns.h"

Rune	*errfname = L"Errors";
Rune	*errtag = L"Errors\tClose!";

void
errs(uchar *s)
{
	Page *p, *parent;
	Rune *r;

	p = findopen(errfname, 1);
	if(p == 0){
		parent = 0;
		if(curt)
			parent = curt->page->parent;
		else if(page){
			if(page->down)
				parent = page;
			else
				parent = page->parent;
		}
		if(parent == 0)
			return;
		p = pageadd(parent, 1);
		textinsert(&p->tag, errtag, 0, 0, 1);
	}
	if(p == 0)
		return;
	r = tmprstr((char*)s);
	textinsert(&p->body, r, p->body.n, 1, 1);
	free(r);
	show(&p->body, p->body.q0);
}
