#include <alef.h>
#include <libg.h>
#include <frame.h>
#include <fcall.h>
#include <bio.h>
#include "dat.h"
#include "fns.h"

void
Row.init(Row *row, Rectangle r)
{
	Rectangle r1;
	Text *t;

	bitblt(&screen, r.min, &screen, r, 0);
	row->r = r;
	row->col = nil;
	row->ncol = 0;
	r1 = r;
	r1.max.y = r1.min.y + font->height;
	t = &row->tag;
	t->init(.File.addtext(t), r1, .Reffont.get(FALSE, FALSE, FALSE, nil));
	t->what = Rowtag;
	t->row = row;
	t->w = nil;
	t->col = nil;
	r1.min.y = r1.max.y;
	r1.max.y += Border;
	bitblt(&screen, r1.min, &screen, r1, 0xF);
	t->insert(0, $"Newcol Kill Putall Dump Exit ", 29, TRUE);
	t->setselect(t->file->nc, t->file->nc);
}

Column*
Row.add(Row *row, Column *c, int x)
{
	Rectangle r, r1;
	Column *d;
	int i;

	d = nil;
	r = row->r;
	r.min.y = row->tag.r.max.y+Border;
	if(x<r.min.x && row->ncol>0){	/*steal 40% of last column by default */
		d = row->col[row->ncol-1];
		x = d->r.min.x + 3*Dx(d->r)/5;
	}
	/* look for column we'll land on */
	for(i=0; i<row->ncol; i++){
		d = row->col[i];
		if(x < d->r.max.x)
			break;
	}
	if(row->ncol > 0){
		if(i < row->ncol)
			i++;	/* new column will go after d */
		r = d->r;
		if(Dx(r) < 100)
			return nil;
		bitblt(&screen, r.min, &screen, r, 0);
		r1 = r;
		r1.max.x = min(x, r.max.x-50);
		if(Dx(r1) < 50)
			r1.max.x = r1.min.x+50;
		d->reshape(r1);
		r1.min.x = r1.max.x;
		r1.max.x = r1.min.x+Border;
		bitblt(&screen, r1.min, &screen, r1, 0xF);
		r.min.x = r1.max.x;
	}
	if(c == nil){
		c = malloc(sizeof(Column));
		c->init(r);
	}else
		c->reshape(r);
	c->row = row;
	c->tag.row = row;
	row->col = realloc(row->col, (row->ncol+1)*sizeof(Column*));
	memmove(row->col+i+1, row->col+i, (row->ncol-i)*sizeof(Column*));
	row->col[i] = c;
	row->ncol++;
	clearmouse();
	return c;
}

void
Row.reshape(Row *row, Rectangle r)
{
	int i, dx, odx;
	Rectangle r1, r2;
	Column *c;

	dx = Dx(r);
	odx = Dx(row->r);
	row->r = r;
	r1 = r;
	r1.max.y = r1.min.y + font->height;
	row->tag.reshape(r1);
	r1.min.y = r1.max.y;
	r1.max.y += Border;
	bitblt(&screen, r1.min, &screen, r1, 0xF);
	r.min.y = r1.max.y;
	r1 = r;
	r1.max.x = r1.min.x;
	for(i=0; i<row->ncol; i++){
		c = row->col[i];
		r1.min.x = r1.max.x;
		if(i == row->ncol-1)
			r1.max.x = r.max.x;
		else
			r1.max.x = r1.min.x+Dx(c->r)*dx/odx;
		r2 = r1;
		r2.max.x = r2.min.x+Border;
		bitblt(&screen, r2.min, &screen, r2, 0xF);
		r1.min.x = r2.max.x;
		c->reshape(r1);
	}
}

void
Row.dragcol(Row *row, Column *c, int)
{
	Rectangle r;
	int i, b, x;
	Point p, op;
	Column *d;

	clearmouse();
	cursorswitch(&boxcursor);
	b = mouse.buttons;
	op = mouse.xy;
	while(mouse.buttons == b)
		frgetmouse();
	cursorswitch(nil);
	if(mouse.buttons){
		while(mouse.buttons)
			frgetmouse();
		return;
	}

	for(i=0; i<row->ncol; i++)
		if(row->col[i] == c)
			goto Found;
	error("can't find column");

  Found:
	if(i == 0)
		return;
	p = mouse.xy;
	if((abs(p.x-op.x)<5 && abs(p.y-op.y)<5))
		return;
	if((i>0 && p.x<row->col[i-1]->r.min.x) || (i<row->ncol-1 && p.x>c->r.max.x)){
		/* shuffle */
		x = c->r.min.x;
		row->close(c, FALSE);
		if(row->add(c, p.x) == nil)	/* whoops! */
		if(row->add(c, x) == nil)		/* WHOOPS! */
		if(row->add(c, -1)==nil){		/* shit! */
			row->close(c, TRUE);
			return;
		}
		c->mousebut();
		return;
	}
	d = row->col[i-1];
	if(p.x < d->r.min.x+20+Scrollwid)
		p.x = d->r.min.x+20+Scrollwid;
	if(p.x > c->r.max.x-20-Scrollwid)
		p.x = c->r.max.x-20-Scrollwid;
	r = d->r;
	r.max.x = c->r.max.x;
	bitblt(&screen, r.min, &screen, r, 0);
	r.max.x = p.x;
	d->reshape(r);
	r = c->r;
	r.min.x = p.x;
	r.max.x = r.min.x;
	r.max.x += Border;
	bitblt(&screen, r.min, &screen, r, 0xF);
	r.min.x = r.max.x;
	r.max.x = c->r.max.x;
	c->reshape(r);
	c->mousebut();
}

void
Row.close(Row *row, Column *c, int dofree)
{
	Rectangle r;
	int i;

	for(i=0; i<row->ncol; i++)
		if(row->col[i] == c)
			goto Found;
	error("can't find column");
  Found:
	r = c->r;
	if(dofree)
		c->closeall();
	memmove(row->col+i, row->col+i+1, (row->ncol-i)*sizeof(Column*));
	row->ncol--;
	row->col = realloc(row->col, row->ncol*sizeof(Column*));
	if(row->ncol == 0){
		bitblt(&screen, r.min, &screen, r, 0);
		return;
	}
	if(i == row->ncol){		/* extend last column right */
		c = row->col[i-1];
		r.min.x = c->r.min.x;
		r.max.x = row->r.max.x;
	}else{			/* extend next window left */
		c = row->col[i];
		r.max.x = c->r.max.x;
	}
	bitblt(&screen, r.min, &screen, r, 0);
	c->reshape(r);
}

Column*
Row.whichcol(Row *row, Point p)
{
	int i;
	Column *c;

	for(i=0; i<row->ncol; i++){
		c = row->col[i];
		if(ptinrect(p, c->r))
			return c;
	}
	return nil;
}

Text*
Row.which(Row *row, Point p)
{
	Column *c;

	if(ptinrect(p, row->tag.all))
		return &row->tag;
	c = row->whichcol(p);
	if(c)
		return c->which(p);
	return nil;
}

Text*
Row.type(Row *row, Rune r, Point p)
{
	Window *w;
	Text *t;

	clearmouse();
	row->lock();
	if(bartflag)
		t = barttext;
	else
		t = row->which(p);
	if(t!=nil && !(t->what==Tag && ptinrect(p, t->scrollr))){
		w = t->w;
		if(w == nil)
			t->type(r);
		else{
			w->lock('K');
			w->type(t, r);
			w->unlock();
		}
	}
	row->unlock();
	return t;
}

int
Row.clean(Row *row)
{
	int clean;
	int i;

	clean = TRUE;
	for(i=0; i<row->ncol; i++)
		clean &= row->col[i]->clean();
	return clean;
}

void
Row.dump(Row *row, byte *file)
{
	int i, j, fd, m, n, dumped;
	uint q0, q1;
	Biobuf *b;
	byte *buf, *a, *fontname;
	Rune *r;
	Column *c;
	Window *w, *w1;
	Text *t;

	if(row->ncol == 0)
		return;
	buf = fbufalloc();
	rescue{
		fbuffree(buf);
		return;
	}
	if(file == nil){
		if(home == nil){
			warning(nil, "can't find file for dump: $home not defined\n");
			raise;
		}
		sprint(buf, "%s/acme.dump", home);
		file = buf;
	}
	fd = create(file, OWRITE, 0600);
	if(fd < 0){
		warning(nil, "can't open %s: %r\n", file);
		raise;
	}
	b = malloc(sizeof(Biobuf));
	b->init(fd, OWRITE);
	r = fbufalloc();
	if(getwd(buf, BUFSIZE))
		b->print("%s\n", buf);
	else
		b->print(".\n");
	b->print("%s\n", fontnames[0]);
	b->print("%s\n", fontnames[1]);
	for(i=0; i<row->ncol; i++){
		c = row->col[i];
		b->print("%11d", 100*(c->r.min.x-row->r.min.x)/Dx(row->r));
		if(i == row->ncol-1)
			b->putc('\n');
		else
			b->putc(' ');
	}
	for(i=0; i<row->ncol; i++){
		c = row->col[i];
		for(j=0; j<c->nw; j++)
			c->w[j]->body.file->dumpid = 0;
	}
	for(i=0; i<row->ncol; i++){
		c = row->col[i];
		for(j=0; j<c->nw; j++){
			w = c->w[j];
			w->commit(&w->tag);
			t = &w->body;
			/* windows owned by others get special treatment */
			if(w->nopen[QWevent] > 0)
				if(w->dumpstr == nil)
					continue;
			/* zeroxes of external windows are tossed */
			if(t->file->ntext > 1)
				for(n=0; n<t->file->ntext; n++){
					w1 = t->file->text[n]->w;
					if(w == w1)
						continue;
					if(w1->nopen[QWevent])
						continue 2;
				}
			fontname = "";
			if(t->reffont->f != font)
				fontname = t->reffont->f->name;
			if(t->file->nname)
				a = runetobyte(t->file->name, t->file->nname);
			else
				a = malloc(1);
			if(t->file->dumpid){
				dumped = FALSE;
				b->print("x%11d %11d %11d %11d %11d %s\n", i, t->file->dumpid,
					w->body.q0, w->body.q1,
					100*(w->r.min.y-c->r.min.y)/Dy(c->r),
					fontname);
			}else if(w->dumpstr){
				dumped = FALSE;
				b->print("e%11d %11d %11d %11d %11d %s\n", i, t->file->dumpid,
					0, 0,
					100*(w->r.min.y-c->r.min.y)/Dy(c->r),
					fontname);
			}else if(strlen(a) == 0){	/* don't save unnamed windows */
				free(a);
				continue;
			}else if((w->dirty==FALSE && access(a, 0)==0) || w->isdir){
				dumped = FALSE;
				t->file->dumpid = w->id;
				b->print("f%11d %11d %11d %11d %11d %s\n", i, w->id,
					w->body.q0, w->body.q1,
					100*(w->r.min.y-c->r.min.y)/Dy(c->r),
					fontname);
			}else{
				dumped = TRUE;
				t->file->dumpid = w->id;
				b->print("F%11d %11d %11d %11d %11d %11d %s\n", i, j,
					w->body.q0, w->body.q1,
					100*(w->r.min.y-c->r.min.y)/Dy(c->r),
					w->body.file->nc, fontname);
			}
			free(a);
			w->ctlprint(buf);
			b->write(buf, strlen(buf));
			m = min(RBUFSIZE, w->tag.file->nc);
			w->tag.file->read(0, r, m);
			n = 0;
			while(n<m && r[n]!='\n')
				n++;
			r[n++] = '\n';
			b->print("%.*S", n, r);
			if(dumped){
				q0 = 0;
				q1 = t->file->nc;
				while(q0 < q1){
					n = q1 - q0;
					if(n > BUFSIZE/UTFmax)
						n = BUFSIZE/UTFmax;
					t->file->read(q0, r, n);
					b->print("%.*S", n, r);
					q0 += n;
				}
			}
			if(w->dumpstr){
				if(w->dumpdir)
					b->print("%s\n%s\n", w->dumpdir, w->dumpstr);
				else
					b->print("\n%s\n", w->dumpstr);
			}
		}
	}
	b->term();
	close(fd);
	free(b);
	fbuffree(buf);
	fbuffree(r);
}

intern
(int, byte*)
rdline(Biobuf *b, int line)
{
	byte *l;

	l = b->rdline('\n');
	if(l)
		line++;
	return (line, l);
}

void
Row.load(Row *row, byte *file, int initing)
{
	int i, j, line, percent, y, nr, nfontr, n, ns, ndumped, dumpid, x, fd;
	Biobuf *b, *bout;
	byte *buf, *l, *t, *fontname;
	Rune *r, rune, *fontr;
	Column *c, *c1, *c2;
	uint q0, q1;
	Rectangle r1, r2;
	Window *w;

	buf = fbufalloc();
	rescue{
		fbuffree(buf);
		return;
	}
	if(file == nil){
		if(home == nil){
			warning(nil, "can't find file for load: $home not defined\n");
			raise;
		}
		sprint(buf, "%s/acme.dump", home);
		file = buf;
	}
	b = Bopen(file, OREAD);
	if(b == nil){
		warning(nil, "can't open load file %s: %r\n", file);
		raise;
	}
	rescue{
		warning(nil, "bad load file %s:%d\n", file, line);
		b->term();
		raise;
	}
	/* current directory */
	(line, l) = rdline(b, 0);
	if(l == nil)
		raise;
	l[b->linelen()-1] = 0;
	if(chdir(l) < 0){
		warning(nil, "can't chdir %s\n", l);
		raise;
	}
	/* global fonts */
	for(i=0; i<2; i++){
		(line, l) = rdline(b, line);
		if(l == nil)
			raise;
		l[b->linelen()-1] = 0;
		if(*l && strcmp(l, fontnames[i])!=0)
			.Reffont.get(i, TRUE, i==0 && initing, l);
	}
	if(initing && row->ncol==0)
		row->init(screen.clipr);
	(line, l) = rdline(b, line);
	if(l == nil)
		raise;
	j = b->linelen()/12;
	if(j<=0 || j>10)
		raise;
	for(i=0; i<j; i++){
		percent = atoi(l+i*12);
		if(percent<0 || percent>=100)
			raise;
		x = row->r.min.x+percent*Dx(row->r)/100;
		if(i < row->ncol){
			if(i == 0)
				continue;
			c1 = row->col[i-1];
			c2 = row->col[i];
			r1 = c1->r;
			r2 = c2->r;
			r1.max.x = x;
			r2.min.x = x+Border;
			if(Dx(r1) < 50 || Dx(r2) < 50)
				continue;
			bitblt(&screen, r1.min, &screen, Rpt(r1.min, r2.max), 0);
			c1->reshape(r1);
			c2->reshape(r2);
			r2.min.x = x;
			r2.max.x = x+Border;
			bitblt(&screen, r2.min, &screen, r2, 0xF);
		}
		if(i >= row->ncol)
			row->add(nil, x);
	}
	for(;;){
		(line, l) = rdline(b, line);
		if(l == nil)
			break;
		dumpid = 0;
		switch(l[0]){
		case 'e':
			if(b->linelen() < 1+5*12+1)
				raise;
			(line, l) = rdline(b, line);	/* ctl line; ignored */
			if(l == nil)
				raise;
			(line, l) = rdline(b, line);	/* directory */
			if(l == nil)
				raise;
			r = nil;
			nr = 0;
			l[b->linelen()-1] = 0;
			if(*l != 0)
				(r, nr) = bytetorune(l);
			(line, l) = rdline(b, line);	/* command */
			if(l == nil)
				raise;
			t = malloc(b->linelen()+1);
			memmove(t, l, b->linelen());
			proc run(nil, t, r, nr, TRUE, nil, nil);
			/* r is freed in run() */
			continue;
		case 'f':
			if(b->linelen() < 1+5*12+1)
				raise;
			fontname = l+1+5*12;
			ndumped = -1;
			break;
		case 'F':
			if(b->linelen() < 1+6*12+1)
				raise;
			fontname = l+1+6*12;
			ndumped = atoi(l+1+5*12+1);
			break;
		case 'x':
			if(b->linelen() < 1+5*12+1)
				raise;
			fontname = l+1+5*12;
			ndumped = -1;
			dumpid = atoi(l+1+1*12);
			break;
		default:
			raise;
		}
		l[b->linelen()-1] = 0;
		if(*fontname)
			(fontr, nfontr) = bytetorune(fontname);
		else
			(fontr, nfontr) = (nil, 0);
		i = atoi(l+1+0*12);
		j = atoi(l+1+1*12);
		q0 = atoi(l+1+2*12);
		q1 = atoi(l+1+3*12);
		percent = atoi(l+1+4*12);
		if(i<0 || i>10)
			raise;
		if(i > row->ncol)
			i = row->ncol;
		c = row->col[i];
		y = c->r.min.y+(percent*Dy(c->r))/100;
		if(y<c->r.min.y || y>=c->r.max.y)
			y = -1;
		if(dumpid == 0)
			w = c->add(nil, nil, y);
		else
			w = c->add(nil, lookid(dumpid, TRUE), y);
		if(w == nil)
			continue;
		w->dumpid = j;
		(line, l) = rdline(b, line);
		if(l == nil)
			raise;
		(r, nr) = bytetorune(l+5*12);
		ns = -1;
		for(n=0; n<nr; n++){
			if(r[n] == '/')
				ns = n;
			if(r[n] == ' ')
				break;
		}
		if(dumpid == 0)
			w->setname(r, n);
		for(; n<nr; n++)
			if(r[n] == '|')
				break;
		w->cleartag();
		w->tag.insert(w->tag.file->nc, r+n+1, nr-(n+1), TRUE);
		free(r);
		if(ndumped >= 0){
			/* simplest thing is to put it in a file and load that */
			sprint(buf, "/tmp/D%d.%.4sacme", getpid(), getuser());
			fd = create(buf, OWRITE|ORCLOSE, 0600);
			if(fd < 0){
				warning(nil, "can't create temp file: %r\n");
				raise;
			}
			bout = malloc(sizeof(Biobuf));
			bout->init(fd, OWRITE);
			for(n=0; n<ndumped; n++){
				rune = b->getrune();
				if(rune == '\n')
					line++;
				if(rune == Beof){
					bout->term();
					free(bout);
					close(fd);
					raise;
				}
				bout->putrune(rune);
			}
			bout->term();
			free(bout);
			w->body.load(0, buf);
			close(fd);
			w->body.file->mod = TRUE;
			for(n=0; n<w->body.file->ntext; n++)
				w->body.file->text[n]->w->dirty = TRUE;
			w->settag();
		}else if(dumpid==0 && r[ns+1]!='+' && r[ns+1]!='-')
			get(&w->body, nil, nil, FALSE, XXX, nil, 0);
		if(fontr){
			fontx(&w->body, nil, nil, 0, 0, fontr, nfontr);
			free(fontr);
		}
		if(q0>w->body.file->nc || q1>w->body.file->nc || q0>q1)
			q0 = q1 = 0;
		w->body.show(q0, q1);
	}
	b->term();
	fbuffree(buf);
}
