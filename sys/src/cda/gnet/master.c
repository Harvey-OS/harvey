#include "geom.h"
#include "thing.h"
#include "text.h"
#include "wire.h"
#include "conn.h"
#include "box.h"
#include "master.h"

extern Vector *masters;

Master::Master(String *ss)
{
	s = ss;
	hasmbb = 0;
	boxes = new BoxList(32);
	macros = new BoxList(8);
	wires = new WireList(32);
	labels = new TextList(32);
}

Master::~Master()
{
	delete wires;
	delete boxes;
	delete macros;
	delete labels;
}

Master::eq(Thing *t)
{
	return s->eq((String *) t);
}

Rectangle Master::findmbb()
{
	if (hasmbb == 0) {
		bb = wires->mbb(Rect(10000,10000,-10000,-10000));
		bb = boxes->mbb(bb);
		boxes->expand(labels);
		hasmbb = 1;
	}
	return bb;
}

void Master::process()
{
	extern kflag,mflag;
	extern strlen(char *);
	extern char *stem;
	findmbb();
	labels->walk(macros,(int(*)(...))BoxList::pins);
	labels->walk(boxes,(int(*)(...))BoxList::parts);
	labels->walk(boxes,(int(*)(...))BoxList::pins);
	labels->walk(wires,(int(*)(...))WireList::nets);
	if (kflag)
		labels->walk(boxes,(int(*)(...))BoxList::kahrs);
	wires->prop();
	wires->prop1();
	macros->nets(wires);
	boxes->nets(wires);
	labels->walk(labels,(int(*)(...))TextList::prop);	/* graw->jraw hack */
	labels->walk(macros,(int(*)(...))BoxList::parts);
	boxes->apply((int *) wires,(int(*)(...))Box::getname);
	macros->absorb(boxes);
	macros->put(stdout);
	if (macros->n > 0 && boxes->n > 0)	/* i wish i knew what this was for */
		fprintf(stdout,".f %s.w\n",stem);
	boxes->put(stdout);
}

void getstring(FILE *inf, char *p)
{
	register c;
	while ((c = getc(inf)) == ' ')
		;
	if (c == '\"')
		while ((c = getc(inf)) != '\"')
			*p++ = c;
	else if (c != '@')
		do
			*p++ = c;
		while ((c = getc(inf)) != '@');
	*p = 0;
}

/* string placement displacements */
#define HALFX	1
#define FULLX	2
#define HALFY	4
#define FULLY	8
#define INVIS	16

/* jraw codes for text placement */
#define	BL	01	/* placement is bottom left corner of string */
#define BC	02	/* bottom center */
#define BR	04	/* bottom right */
#define RC	010	/* right center */
#define TR	020	/* top right */
#define TC	040	/* top center */
#define TL	0100	/* top left */
#define LC	0200	/* left center */

jtog(int i)		/* converts jraw placement codes */
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

Master::get(FILE *inf)
{
	int c,junk,style1;
	char buf0[80],buf1[80],buf2[80];
	FILE *incf;
	Rectangle R;
	Master *m;
	for (c = getc(inf);; c = getc(inf)) {
		switch (c) {
		case EOF:
		case 'e':
			return 1;
		case 'r':
			getstring(inf,buf0);
			sprintf(buf1,"/lib/graw/%s",buf0);
			if ((incf = fopen(buf0,"r")) != 0 || (incf = fopen(buf1,"r")) != 0) {
				get(incf);
				fclose(incf);
			}
			else
				fprintf(stderr,"can't include %s\n",buf0);
			break;
		case 'm':
			getstring(inf,buf0);
			masters->append(m = new Master(new String(buf0)));
			m->get(inf);
			break;
		case 'i':
			getstring(inf,buf0);
			fscanf(inf," %d %d",&X,&Y);
			boxes->append(new Inst(new String(buf0),P));
			break;
		case 'w':
		case 'l':
			fscanf(inf,"%d %d %d %d",&X,&Y,&U,&V);
			wires->append(new Wire(R));
			break;
		case 'b':
			fscanf(inf,"%d %d %d %d",&X,&Y,&U,&V);
			boxes->append(new Box(R));
			break;
		case 'z':
			fscanf(inf,"%d %d %d %d",&X,&Y,&U,&V);
			macros->append((Box *) new Macro(R));
			break;
		case 't':
			fscanf(inf," %[^@]@ %d %[^@]@ %d %d %d",
				buf0,&style1,buf1,&junk,&X,&Y);
			labels->append(new Text(P,new String(buf0),new String(buf1),jtog(style1)));
			break;
		case 's':
			getstring(inf,buf0);
			fscanf(inf," %d %d %d",&style1,&X,&Y);
			if (buf0[0] != 0)
				labels->append(new Text(P,new String(buf0),new String(buf1),style1));
			break;
		}
		buf0[0] = buf1[0] = buf2[0] = 0;
	}
}

Inst::Inst(String *ss, Point p)
{
	s = ss;
	m = 0;
	part = 0;
	P = p;
}

Rectangle Inst::mbb(Rectangle r)
{
	if (m == 0)		/* haven't found my master yet */
		if ((m = (Master *) masters->lookup(s)) == 0) {
			fprintf(stderr,"undefined master: %s\n",s->s);
			return r;
		}
	R = (m->findmbb()).translate(P);
	return Box::mbb(r);
}

void Inst::put(FILE *ouf)
{
	static instnum=0;
	if (suffix || part) {
		Box::put(ouf);
		return;
	}
	fprintf(ouf,".c	$I%04d	%s\n",instnum++,s->s);
	pins->put(ouf);
}

void Inst::expand(Vector *v)
{
	register i;
	register Text *t;
	register TextList *l = m->labels;
	for (i = 0; i < l->n; i++) {
		t = (Text *) l->a[i];
		v->append(new Text(P+t->p,new String(t->s->s),t->t,t->style));
	}
}
