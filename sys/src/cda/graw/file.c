#include <u.h>
#include <libc.h>
#include <libg.h>
#include <stdio.h>
#include "thing.h"

extern Font *tiny;
extern Dict *files;

File *
newfile(char *s) /* s presumed unique */
{
	File *f;
	f = (File *) calloc(1,sizeof(File));
	f->name = s;
	f->d = (Dict *) calloc(1,sizeof(Dict));
	return f;
}

initfile(File *f)
{
	int i;
	Line *l;
	Dict *r,*d = f->d;
	File *x;
	if (d->n > 0) {
		free(d->key);
		free(d->val);
		d->n = 0;
	}
	for (l = f->dl; l; l = l->next)		/* start with ref dictionaries */
		if (l->type == REF) {
			if ((x = (File *) lookup(((Ref *)l)->filename,files)) == 0)
				readfile(((Ref *)l)->filename);
			r = ((File *)lookup(((Ref *)l)->filename,files))->d;
			for (i = 0; i < r->n; i++)
				if (r->key[i])
					assign(r->key[i],(char *)r->val[i],d);
		}
	for (l = f->dl; l; l = l->next)		/* add local master defs */
		if (l->type == MASTER)
			assign(((Master *)l)->name,(char *) l,d);
	mbb(f->dl,d);
}

inint(FILE *fp)
{
	int c,i=0,sign=1;
	while ((c = fgetc(fp)) == ' ')		/* really special case */
		;
	if (c == '-') {
		sign = -1;
		c = fgetc(fp);			/* better be a digit */
	}
	for (; c >= '0' && c <= '9'; c = fgetc(fp))
		i = 10*i + (c-'0');
	return i*sign;
}

Point
inpoint(FILE *fp)
{
	Point p;
	p.x = inint(fp);
	p.y = inint(fp);
	if (showgrey) {
		p.x <<= 1;
		p.y <<= 1;
	}
	return p;
}

Rectangle
inrect(FILE *fp)
{
	Rectangle r;
	r.min = inpoint(fp);
	r.max = inpoint(fp);
	return r;
}

static char inbuf[80];

char *
instring(FILE *fp)
{
	int c;
	char *s=inbuf;
	while ((c = fgetc(fp)) == ' ')
		;
	if (c == '"')
		c = fgetc(fp);
	for (; c != '@' && c != '"'; c = fgetc(fp))
		*s++ = c;
	*s = 0;
	if (s > &inbuf[80])
		print("blew internal buffer\n");
	return inbuf;
}

jtog(int i)				/* converts jraw placement codes */
{
	int o = 0;
	if (i == 0)
		return INVIS;
	if (i&(BC|TC))
		o |= HALFX;
	else if (i&(TR|RC|BR))
		o |= FULLX;
	if (i&(LC|RC))
		o |= HALFY;
	else if (i&(BL|BC|BR))
		o |= FULLY;
	return o;
}

int nest;

Line *
inlist(FILE *fp)
{
	int c,i;
	char *s;
	Point p;
	Line *l=0;
	while (1) {
		c=fgetc(fp);
		switch (c) {
		case '\n':
			break;
		default:
			do
				c = fgetc(fp);
			while (c != '\n');
			break;
		case 'w':
		case 'l':
			l = newline(inpoint(fp),l);
			l->Q = inpoint(fp);
			break;
		case 'b':
			l = newbox(inpoint(fp),l);
			l->Q = inpoint(fp);
			break;
		case 'd':
			l = newdots(inpoint(fp),l);
			l->Q = inpoint(fp);
			break;
		case 'z':
			l = newmacro(inpoint(fp),l);
			l->Q = inpoint(fp);
			break;
		case 's':
			s = instring(fp);
			i = inint(fp);
			p = inpoint(fp);
			if (*s != 0) {
				l = newstring(p,i,l);
				setstring((String *)l,s);
			}
			break;
		case 't':		/* turn a jraw text into two graw strings */
			s = instring(fp);
			i = jtog(inint(fp));
			p = Pt(0,(showgrey ? GRID*2 : GRID));
			l = newstring(p,i,l);
			setstring((String *)l,s);
			s = instring(fp);
			inint(fp);
			p = inpoint(fp);
			if (*s != 0) {		/* gad, what a mess */
				l = newstring(add(p,l->P),i,l);
				setstring((String *)l,s);
				l->next->P = p;
			}
			else			/* watch carefully */
				l->P = p;
			break;
		case 'r':
			l = newref(intern(instring(fp)),l);
			break;
		case 'i':
			s = intern(instring(fp));
			l = newinst(inpoint(fp),s,l);
			break;
		case 'm':
			nest = 1;
			l = newmaster(intern(instring(fp)),l);
			((Master *)l)->dl = inlist(fp);
			break;
		case 'e':
			if (nest == 0)
				print("found end of no macro\n");
			nest = 0;
		case -1:
			return l;
		}
	}
}
