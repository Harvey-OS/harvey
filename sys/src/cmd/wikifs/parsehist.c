/*
 * Read a Wiki history file.
 * It's a title line then a sequence of Wiki files separated by headers.
 *
 * Ddate/time
 * #body
 * #...
 * #...
 * #...
 * etc.
 */

#include <u.h>
#include <libc.h>
#include <bio.h>
#include <String.h>
#include <thread.h>
#include "wiki.h"

static char*
Brdwline(void *vb, int sep)
{
	Biobufhdr *b;
	char *p;

	b = vb;
	if(Bgetc(b) == '#'){
		if(p = Brdline(b, sep))
			p[Blinelen(b)-1] = '\0';
		return p;
	}else{
		Bungetc(b);
		return nil;	
	}
}

Whist*
Brdwhist(Biobuf *b)
{
	int i, current, conflict, c, n;
	char *author, *comment, *p, *title;
	ulong t;
	Wdoc *w;
	Whist *h;

	if((p = Brdline(b, '\n')) == nil){
		werrstr("short read: %r");
		return nil;
	}

	p[Blinelen(b)-1] = '\0';
	p = strcondense(p, 1);
	title = estrdup(p);

	w = nil;
	n = 0;
	t = -1;
	author = nil;
	comment = nil;
	conflict = 0;
	current = 0;
	while((c = Bgetc(b)) != Beof){
		if(c != '#'){
			p = Brdline(b, '\n');
			if(p == nil)
				break;
			p[Blinelen(b)-1] = '\0';

			switch(c){
			case 'D':
				t = strtoul(p, 0, 10);
				break;
			case 'A':
				free(author);
				author = estrdup(p);
				break;
			case 'C':
				free(comment);
				comment = estrdup(p);
				break;
			case 'X':
				conflict = 1;
			}
		} else {	/* c=='#' */
			Bungetc(b);
			if(n%8 == 0)
				w = erealloc(w, (n+8)*sizeof(w[0]));
			w[n].time = t;
			w[n].author = author;
			w[n].comment = comment;
			comment = nil;
			author = nil;
			w[n].wtxt = Brdpage(Brdwline, b);
			w[n].conflict = conflict;
			if(w[n].wtxt == nil)
				goto Error;
			if(!conflict)
				current = n;
			n++;
			conflict = 0;
			t = -1;
		}
	}
	if(w==nil)
		goto Error;

	free(comment);
	free(author);
	h = emalloc(sizeof *h);
	h->title = title;
	h->doc = w;
	h->ndoc = n;
	h->current = current;
	incref(h);
	setmalloctag(h, getcallerpc(&b));
	return h;

Error:
	free(title);
	free(author);
	free(comment);
	for(i=0; i<n; i++){
		free(w[i].author);
		free(w[i].comment);
		freepage(w[i].wtxt);
	}
	free(w);
	return nil;
}

