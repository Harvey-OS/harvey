#include <u.h>
#include <libc.h>
#include <ctype.h>
#include <bio.h>
#include <thread.h>
#include "object.h"
#include "parse.h"
#include "catset.h"

int	fflag;

void
listfiles(Object *o)
{
	int i;

	if(o->type == File){
		print("%s\n", o->value);
		return;
	}
	for(i = 0; i < o->nchildren; i++)
		if(o->children[i]->parent == o)
			listfiles(o->children[i]);
}

int
indent(char *lp, int ln, int n, char *buf) {
	int sln;
	char *p, c;

	sln = ln;
	if (ln <= 0)
		return 0;
	if (n < 0)
		n = -n;
	else {
		if (ln < 4*n)
			goto out;
		memset(lp, ' ', 4*n);
		lp += 4*n;
		ln -= 4*n;
	}
	if(p = buf) while (ln > 1) {
		c = *p++;
		if(c == '\0')
			break;
		if(c == '~')
			continue;
		*lp++ = c;
		ln--;
		if (c == '\n' && p[1]) {
			if (ln < 4*n)
				break;
			memset(lp, ' ', 4*n);
			lp += 4*n;
			ln -= 4*n;
		}
	}
	*lp = '\0';
out:
	return sln - ln;
}

long
printchildren(char *lp, int ln, Object *o) {
	int i, r;
	char *sp;

	sp = lp;
	if (o->flags & Sort) {
		childsort(o);
		o->flags &= ~Sort;
	}
	for(i = 0; i < o->nchildren && ln > 0; i++){
		r = snprint(lp, ln, "%d\n", o->children[i]->tabno);
		lp += r;
		ln -= r;
	}
	return lp - sp;
}

long
printminiparentage(char *lp, int ln, Object *o) {
	char *p, c;
	int r, sln;

	if (ln <= 0) return 0;
	*lp = '\0';
	if (o == 0 || o->type == Root)
		return 0;
	sln = ln;
	if(o->orig) o = o->orig;
	r = printminiparentage(lp, ln, o->parent);
	lp += r;
	ln -= r;
	if (ln <= 0) return 0;
	if(o->value && o->type != File){
		if(r && o->value && ln > 1){
			*lp++ = '/';
			ln--;
		}
		p = o->value;
		while(ln > 0){
			c = *p++;
	    		if(c == '\n' || c == '\0')
	    			break;
			if(c == '~')
				continue;
			*lp++ = c;
			ln--;
		}
	}
	if(ln > 0)
		*lp = '\0';
	return sln - ln;
}

long
printparentage(char *lp, int ln, Object *o) {
	int i;
	int r, k, sln;

	if(ln <= 0)
		return 0;
	*lp = '\0';
	if(o == 0 || o->type == Root)
		return 0;
	if(0)fprint(2, "parentage 0x%p %d type %d value 0x%p parent 0x%p %d\n", o, o->tabno, o->type, o->value, o->parent, o->parent->tabno);
	if(o->orig){
		if(0)fprint(2, "parentage 0x%p %d type %d orig %d type %d parent 0x%p %d\n", o, o->tabno, o->type, o->orig->tabno, o->orig->type, o->orig->parent, o->orig->parent->tabno);
		o = o->orig;
	}
	sln = ln;
	r = printparentage(lp, ln, o->parent);
	lp += r; ln -= r;
	if(o->type == File && fflag == 0){
		if(ln > 0)
			*lp = '\0';
		return sln - ln;
	}
	if(o->value && *o->value && ln > 0){
		if(o->type == Category){
			if(o->parent == root){
				r = snprint(lp, ln, "category: ");
				lp += r; ln -= r;
			}else{
				for(k = Ntoken; k < ntoken; k++)
					if(catseteq(&o->categories,&tokenlist[k].categories)){
						r = snprint(lp, ln, "%s: ", tokenlist[k].name);
						lp += r; ln -= r;
						break;
					}
			}
		}else{
			r = snprint(lp, ln, "%s: ", tokenlist[o->type].name);
			lp += r; ln -= r;
		}
		if(ln <= 0)
			return sln - ln;
		if(o->num){
			r = snprint(lp, ln, "%2d. ", o->num);
			lp += r; ln -= r;
		}
		if(ln <= 0)
			return sln - ln;
		r = indent(lp, ln, -1, o->value);
		lp += r; ln -= r;
		if(ln > 1){
			*lp++ = '\n';
			ln--;
		}
	}else{
		if(0)fprint(2, "parentage 0x%p %d type %d no value\n", o, o->tabno, o->type);
	}
	for(i = 0; i < o->nchildren && ln > 0; i++)
		switch(o->children[i]->type){
		case Performance:
		case Soloists:
		case Lyrics:
			r = snprint(lp, ln, "%s: ", tokenlist[o->children[i]->type].name);
			lp += r; ln -= r;
			if(ln <= 0)
				break;
			r = indent(lp, ln, -1, o->children[i]->value);
			lp += r; ln -= r;
			if(ln > 1){
				*lp++ = '\n';
				ln--;
			}
			break;
		case Time:
			r = snprint(lp, ln, "%s: %s\n", "duration", o->children[i]->value);
			lp += r; ln -= r;
			break;
		case File:
			if(fflag){
				r = snprint(lp, ln, "%s: %s\n", "file", o->children[i]->value);
				lp += r; ln -= r;
			}
			break;
		default:
			break;
		}
	if(ln > 0)
		*lp = '\0';
	return sln - ln;
}

long
printparent(char *lp, int ln, Object *o) {
	return snprint(lp, ln, "%d", o->parent->tabno);
}

long
printkey(char *lp, int ln, Object *o) {
	return snprint(lp, ln, "%s", o->key?o->key:o->value);
}

long
printtype(char *lp, int ln, Object *o) {
	return snprint(lp, ln, "%s", tokenlist[o->type].name);
}

long
printtext(char *lp, int ln, Object *o) {
	return snprint(lp, ln, "%s", o->value?o->value:o->key);
}

long
printfulltext(char *lp, int ln, Object *o) {
	char *sp, *p, *q;
	int i, j, k, c, depth;
	Object *oo;

	depth = 0;
	sp = lp;
	switch(o->type){
	case Category:
		if(o->parent == root){
			j = snprint(lp, ln, "category:");
			lp += j; ln -= j;
		}else{
			for(k = Ntoken; k < ntoken; k++)
				if(catseteq(&o->categories, &tokenlist[k].categories)){
					j = snprint(lp, ln, "%s:", tokenlist[k].name);
					lp += j; ln -= j;
					break;
				}
		}
		if(ln <= 0)
			return lp - sp;
		p = o->value;
		if(p == nil)
			p = o->key;
		if((q = strchr(p, '\n')) && q[1] != '\0'){
			// multiple lines
			*lp++ = '\n'; ln--;
			if(ln <= 0)
				break;
			j = indent(lp, ln, depth+1, p);
			lp += j; ln -= j;
		}else{
			*lp++ = ' '; ln--;
			while((c=*p++) && ln > 0){
				if(c == '~')
					continue;
				*lp++ = c;
				ln--;
				if(c == '\n')
					break;
			}
		}
		break;
	case Track:
	case Part:
	case Recording:
	case Root:
	case Search:
	case Work:
		j = snprint(lp, ln, "%s:", tokenlist[o->type].name);
		lp += j; ln -= j;
		if(ln <= 0)
			break;
		if(o->num){
			j = snprint(lp, ln, " %2d.", o->num);
			lp += j; ln -= j;
		}
		if(ln <= 0)
			break;
		p = o->value;
		if(p == nil)
			p = o->key;
		if((q = strchr(p, '\n')) && q[1] != '\0'){
			// multiple lines
			*lp++ = '\n'; ln--;
			if(ln <= 0)
				break;
			j = indent(lp, ln, depth+1, p);
			lp += j; ln -= j;
		}else{
			*lp++ = ' '; ln--;
			while((c =*p++) && ln > 0){
				if(c == '~')
					continue;
				*lp++ = c;
				ln--;
				if(c == '\n')
					break;
			}
		}
	default:
		break;
	}
	depth++;
	for(i = 0; i < o->nchildren && ln > 0; i++){
		oo = o->children[i];
		switch(oo->type){
		case Lyrics:
		case Performance:
		case Soloists:
		case Time:
			if (ln <= 4*depth + 1)
				break;
			*lp++ = '\n'; ln--;
			memset(lp, ' ', 4*depth);
			lp += 4*depth;
			ln -= 4*depth;
			if(ln <= 0)
				break;
			j = snprint(lp, ln, "%s:", tokenlist[oo->type].name);
			lp += j; ln -= j;
			if(ln <= 0)
				break;
			p = oo->value;
			if(ln <= 1)
				break;
			if((q = strchr(p, '\n')) && q[1] != '\0'){
				// multiple lines
				*lp++ = '\n'; ln--;
				j = indent(lp, ln, depth+1, p);
				lp += j; ln -= j;
			}else{
				*lp++ = ' '; ln--;
				while((c =*p++) && ln > 0){
					if(c == '~')
						continue;
					*lp++ = c;
					ln--;
					if(c == '\n')
						break;
				}
			}
		}
	}
	*lp = '\0';
	return lp - sp;
}

long
printfiles(char *lp, int ln, Object *o) {
	int i, r;
	char *sp;

	sp = lp;
	if (o->type == File)
		lp += snprint(lp, ln, "%d	%s\n", o->tabno, o->value);
	else {
		for (i = 0; i < o->nchildren && ln > 0; i++){
			r = printfiles(lp, ln, o->children[i]);
			lp += r;
			ln -= r;
		}
	}
	return lp - sp;
}

long
printdigest(char *lp, int ln, Object *o) {
	char *p;
	int j, c, k;
	char *sp;

	sp = lp;
	switch(o->type){
	case Category:
		if (o->parent == root) {
			j = snprint(lp, ln, "category: ");
			lp += j; ln -= j;
		} else {
			for (k = Ntoken; k < ntoken; k++)
				if (catseteq(&o->categories,& tokenlist[k].categories)) {
					j = snprint(lp, ln, "%s: ", tokenlist[k].name);
					lp += j; ln -= j;
//					break;
				}
		}
		p = o->value;
		if (p == 0) p = o->key;
		while ((c=*p++) && c != '\n' && ln > 0) {
			if(c == '~')
				continue;
			*lp++ = c;
			ln--;
		}
		break;
	case Track:
	case Part:
	case Recording:
	case Root:
	case Search:
	case Work:
		j = snprint(lp, ln, "%s: ", tokenlist[o->type].name);
		lp += j; ln -= j;
		if (o->num) {
			j = snprint(lp, ln, "%2d. ", o->num);
			lp += j; ln -= j;
		}
		p = o->value;
		if (p == 0) p = o->key;
		while ((c = *p++) && c != '\n' && ln > 0) {
			if(c == '~')
				continue;
			*lp++ =  c;
			ln--;
		}
	default:
		break;
	}
	if(ln)
		*lp = '\0';
	return lp - sp;
}

#ifdef UNDEF

void
printtree(Object *o, int ind) {
	char *p;
	char buf[2048];
	int i;

	sprintf(buf, "%s {\n", tokenlist[o->type].name);
	indent(ind, buf);
	ind++;
	if ((p = o->value)) {
		sprintf(buf, "%s\n", p);
		indent(ind, buf);
	}
	for (i = 0; i < o->nchildren; i++)
		printtree(o->children[i], ind);
	indent(--ind, "}\n");
}

void
mapdump(Object *o, int depth, char *inheritance) {
	Object *oo;
	char *data;
	int n, l;

	if (o == root) {
	    depth = 0;
	    inheritance = "";
	    data = NULL;
	} else {
	    if (o->value) {
		l = nlines(o->value);
	        n = strlen(inheritance) +
		    l * (strlen(tokenlist[o->type].name) + 2) +
		    strlen(o->value) + 2;
		data = mymalloc(NULL, n);
		strcpy(data, inheritance);
		p = data + strlen(inheritance);
		q = o->value;
		while (*q) {
		    p += sprintf(p, "%s:\t", tokenlist[o->type].name);
		    do {
			*p++ = *q;
		    while (*q++ != '\n');
		}
		if (p[-1] != '\n') *p++ = '\n';
		*p = 0;
		inheritance = data;
	    }
	    indent(depth, inheritance);
	}
	c = 0;
}

#endif
