#include <alef.h>
#include <libg.h>
#include <frame.h>
#include <fcall.h>
#include "dat.h"
#include "fns.h"

Buffer	snarfbuf;

void	del(Text*, Text*, Text*, int, int, Rune*, int);
void	delcol(Text*, Text*, Text*, int, int, Rune*, int);
void	dump(Text*, Text*, Text*, int, int, Rune*, int);
void	exit(Text*, Text*, Text*, int, int, Rune*, int);
void	fontx(Text*, Text*, Text*, int, int, Rune*, int);
void	get(Text*, Text*, Text*, int, int, Rune*, int);
void	id(Text*, Text*, Text*, int, int, Rune*, int);
void	incl(Text*, Text*, Text*, int, int, Rune*, int);
void	kill(Text*, Text*, Text*, int, int, Rune*, int);
void	local(Text*, Text*, Text*, int, int, Rune*, int);
void	look(Text*, Text*, Text*, int, int, Rune*, int);
void	newcol(Text*, Text*, Text*, int, int, Rune*, int);
void	paste(Text*, Text*, Text*, int, int, Rune*, int);
void	put(Text*, Text*, Text*, int, int, Rune*, int);
void	putall(Text*, Text*, Text*, int, int, Rune*, int);
void	send(Text*, Text*, Text*, int, int, Rune*, int);
void	sort(Text*, Text*, Text*, int, int, Rune*, int);
void	undo(Text*, Text*, Text*, int, int, Rune*, int);
void	zeroxx(Text*, Text*, Text*, int, int, Rune*, int);

aggr Exectab
{
	Rune	*name;
	void	(*fn)(Text*, Text*, Text*, int, int, Rune*, int);
	int		mark;
	int		flag1;
	int		flag2;
};

Exectab exectab[] = {
	{ $"Cut",		cut,		TRUE,	TRUE,	TRUE	},
	{ $"Del",		del,		FALSE,	FALSE,	XXX		},
	{ $"Delcol",	delcol,	FALSE,	XXX,		XXX		},
	{ $"Delete",	del,		FALSE,	TRUE,	XXX		},
	{ $"Dump",	dump,	FALSE,	TRUE,	XXX		},
	{ $"Exit",		exit,		FALSE,	XXX,		XXX		},
	{ $"Font",		fontx,	FALSE,	XXX,		XXX		},
	{ $"Get",		get,		FALSE,	TRUE,	XXX		},
	{ $"ID",		id,		FALSE,	XXX,		XXX		},
	{ $"Incl",		incl,		FALSE,	XXX,		XXX		},
	{ $"Kill",		kill,		FALSE,	XXX,		XXX		},
	{ $"Load",		dump,	FALSE,	FALSE,	XXX		},
	{ $"Local",		local,	FALSE,	XXX,		XXX		},
	{ $"Look",		look,		FALSE,	XXX,		XXX		},
	{ $"New",		new,		FALSE,	XXX,		XXX		},
	{ $"Newcol",	newcol,	FALSE,	XXX,		XXX		},
	{ $"Paste",		paste,	TRUE,	XXX,		XXX		},
	{ $"Put",		put,		FALSE,	XXX,		XXX		},
	{ $"Putall",	putall,	FALSE,	XXX,		XXX		},
	{ $"Redo",		undo,	FALSE,	FALSE,	XXX		},
	{ $"Send",		send,	TRUE,	XXX,		XXX		},
	{ $"Snarf",		cut,		FALSE,	TRUE,	FALSE	},
	{ $"Sort",		sort,		FALSE,	XXX,		XXX		},
	{ $"Undo",		undo,	FALSE,	TRUE,	XXX		},
	{ $"Zerox",	zeroxx,	FALSE,	XXX,		XXX		},
	{ nil, 			nil,		0,		0,		0		},
};

Exectab*
lookup(Rune *r, int n)
{
	Exectab *e;
	int nr;

	(r, n) = skipbl(r, n);
	if(n == 0)
		return nil;
	(nil, nr) = findbl(r, n);
	nr = n-nr;
	for(e=exectab; e->name; e++)
		if(runeeq(r, nr, e->name, runestrlen(e->name)) == TRUE)
			return e;
	return nil;
}

void
execute(Text *t, uint aq0, uint aq1, int external, Text *argt)
{
	uint q0, q1;
	Rune *r, *s, *dir;
	byte *b, *a, *aa;
	Exectab *e;
	int c, n, f;

	q0 = aq0;
	q1 = aq1;
	if(q1 == q0){	/* expand to find word (actually file name) */
		/* if in selection, choose selection */
		if(t->q1>t->q0 && t->q0<=q0 && q0<=t->q1){
			q0 = t->q0;
			q1 = t->q1;
		}else{
			while(q1<t->file->nc && isfilec(c=t->readc(q1)) && c!=':')
				q1++;
			while(q0>0 && isfilec(c=t->readc(q0-1)) && c!=':')
				q0--;
			if(q1 == q0)
				return;
		}
	}
	r = runemalloc(q1-q0);
	t->file->read(q0, r, q1-q0);
	e = lookup(r, q1-q0);
	if(!external && t->w!=nil && t->w->nopen[QWevent]>0){
		f = 0;
		if(e)
			f |= 1;
		if(q0!=aq0 || q1!=aq1){
			t->file->read(aq0, r, aq1-aq0);
			f |= 2;
		}
		(aa, a) = getbytearg(argt, TRUE, TRUE);
		if(a){	
			if(strlen(a) > EVENTSIZE){	/* too big; too bad */
				free(aa);
				free(a);
				warning(nil, "`argument string too long\n");
				return;
			}
			f |= 8;
		}
		c = 'x';
		if(t->what == Body)
			c = 'X';
		n = aq1-aq0;
		if(n <= EVENTSIZE)
			t->w->event("%c%d %d %d %d %.*S\n", c, aq0, aq1, f, n, n, r);
		else
			t->w->event("%c%d %d %d 0 \n", c, aq0, aq1, f, n);
		if(q0!=aq0 || q1!=aq1){
			n = q1-q0;
			t->file->read(q0, r, n);
			if(n <= EVENTSIZE)
				t->w->event("%c%d %d 0 %d %.*S\n", c, q0, q1, n, n, r);
			else
				t->w->event("%c%d %d 0 0 \n", c, q0, q1, n);
		}
		if(a){
			t->w->event("%c0 0 0 %d %s\n", c, utflen(a), a);
			if(aa)
				t->w->event("%c0 0 0 %d %s\n", c, utflen(aa), aa);
			else
				t->w->event("%c0 0 0 0 \n", c);
		}
		free(r);
		free(aa);
		free(a);
		return;
	}
	if(e){
		if(e->mark && seltext!=nil)
		if(seltext->what == Body){
			seq++;
			seltext->w->body.file->mark();
		}
		(s, n) = skipbl(r, q1-q0);
		(s, n) = findbl(s, n);
		(s, n) = skipbl(s, n);
		(*e->fn)(t, seltext, argt, e->flag1, e->flag2, s, n);
		free(r);
		return;
	}

	b = runetobyte(r, q1-q0);
	free(r);
	(dir, n) = dirname(t, nil, 0);
	if(n==1 && dir[0]=='.'){	/* sigh */
		free(dir);
		dir = nil;
		n = 0;
	}
	(aa, a) = getbytearg(argt, TRUE, TRUE);
	if(t->w)
		t->w->inc();
	proc run(t->w, b, dir, n, TRUE, aa, a);
}

byte*
printarg(Text *argt, uint q0, uint q1)
{
	byte *buf;

	if(argt->what!=Body || argt->file->name==nil)
		return nil;
	buf = malloc(argt->file->nname+32);
	if(q0 == q1)
		sprint(buf, "%.*S:#%d", argt->file->nname, argt->file->name, q0);
	else
		sprint(buf, "%.*S:#%d,#%d", argt->file->nname, argt->file->name, q0, q1);
	return buf;
}

(byte*, Rune*, int)
getarg(Text *argt, int doaddr, int dofile)
{
	Rune *r;
	int n;
	Expand e;
	byte *a;

	if(argt == nil)
		return (nil, nil, 0);
	a = nil;
	if(expand(argt, argt->q0, argt->q1, &e)){
		free(e.bname);
		if(e.nname && dofile){
			e.name = runerealloc(e.name, e.nname+1);
			if(doaddr)
				a = printarg(argt, e.q0, e.q1);
			return (a, e.name, e.nname);
		}
		free(e.name);
	}else{
		e.q0 = argt->q0;
		e.q1 = argt->q1;
	}
	n = e.q1 - e.q0;
	r = runemalloc(n+1);
	argt->file->read(e.q0, r, n);
	if(doaddr)
		a = printarg(argt, e.q0, e.q1);
	return(a, r, n);
}

(byte*, byte*)
getbytearg(Text *argt, int doaddr, int dofile)
{
	Rune *r;
	int n;
	byte *b, *aa;

	(aa, r, n) = getarg(argt, doaddr, dofile);
	if(r == nil)
		return (nil, nil);
	b = runetobyte(r, n);
	free(r);
	return (aa, b);
}

void
newcol(Text *et, Text*, Text*, int, int, Rune*, int)
{
	Column *c;

	c = et->row->add(nil, -1);
	if(c)
		c->add(nil, nil, -1)->settag();
}

void
delcol(Text *et, Text*, Text*, int, int, Rune*, int)
{
	if(et->col!=nil && et->col->clean())
		et->col->row->close(et->col, TRUE);
}

void
del(Text *et, Text*, Text*, int flag1, int, Rune*, int)
{
	if(et->col==nil || et->w == nil)
		return;
	if(flag1 || et->w->body.file->ntext>1 || et->w->clean(FALSE))
		et->col->close(et->w, TRUE);
}

void
sort(Text *et, Text*, Text*, int, int, Rune*, int)
{
	if(et->col)
		et->col->sort();
}

void
undo(Text *et, Text*, Text*, int flag1, int, Rune*, int)
{
	if(et)
		et->w->undo(flag1);
}

byte*
getname(Text *t, Text *argt, Rune *arg, int narg, int isput)
{
	byte *s;
	Rune *r, *dir;
	int i, n, ndir, promote;

	(nil, r, n) = getarg(argt, FALSE, TRUE);
	promote = FALSE;
	if(r == nil)
		promote = TRUE;
	else if(isput){
		/* if are doing a Put, want to synthesize name even for non-existent file */
		/* best guess is that file name doesn't contain a slash */
		promote = TRUE;
		for(i=0; i<n; i++)
			if(r[i] == '/'){
				promote = FALSE;
				break;
			}
		if(promote){
			t = argt;
			arg = r;
			narg = n;
		}
	}
	if(promote){
		n = narg;
		if(n <= 0){
			s = runetobyte(t->file->name, t->file->nname);
			return s;
		}
		/* prefix with directory name if necessary */
		dir = nil;
		ndir = 0;
		if(n>0 && arg[0]!='/'){
			(dir, ndir) = dirname(t, nil, 0);
			if(n==1 && dir[0]=='.'){	/* sigh */
				free(dir);
				dir = nil;
				ndir = 0;
			}
		}
		if(dir){
			r = runemalloc(ndir+n+1);
			runemove(r, dir, ndir);
			free(dir);
			runemove(r+ndir, arg, n);
			n += ndir;
		}else{
			r = runemalloc(n+1);
			runemove(r, arg, n);
		}
	}
	s = runetobyte(r, n);
	free(r);
	if(strlen(s) == 0){
		free(s);
		s = nil;
	}
	return s;
}

void
zeroxx(Text *et, Text *t, Text*, int, int, Rune*, int)
{
	Window *nw;
	int c, locked;

	locked = FALSE;
	if(t!=nil && t->w!=nil && t->w!=et->w){
		locked = TRUE;
		c = 'M';
		if(et->w)
			c = et->w->owner;
		t->w->lock(c);
	}
	if(t == nil)
		t = et;
	if(t==nil || t->w==nil)
		return;
	t = &t->w->body;
	if(t->w->isdir)
		warning(nil, "%.*S is a directory; Zerox illegal\n", t->file->nname, t->file->name);
	else{
		nw = t->w->col->add(nil, t->w, -1);
		/* ugly: fix locks so w->unlock works */
		nw->lock1(t->w->owner);
	}
	if(locked)
		t->w->unlock();
}

void
get(Text *et, Text *t, Text *argt, int flag1, int, Rune *arg, int narg)
{
	byte *name;
	Rune *r;
	int i, n, dirty;
	Window *w;
	Text *u;
	Dir d;

	if(flag1)
		if(et==nil || et->w==nil)
			return;
	if(!et->w->isdir && (et->w->body.file->nc>0 && !et->w->clean(TRUE)))
		return;
	w = et->w;
	t = &w->body;
	name = getname(t, argt, arg, narg, FALSE);
	if(name == nil){
		warning(nil, "no file name\n");
		return;
	}
	if(t->file->ntext>1 && dirstat(name, &d)==0 && d.qid.path & CHDIR){
		warning(nil, "%s is a directory; can't read with multiple windows on it\n", name);
		return;
	}
	(r, n) = bytetorune(name);
	for(i=0; i<t->file->ntext; i++){
		u = t->file->text[i];
		/* second and subsequent calls with zero an already empty buffer, but OK */
		u->reset();
		u->w->dirfree();
	}
	t->load(0, name);
	if(runeeq(r, n, t->file->name, t->file->nname)){
		t->file->mod = FALSE;
		dirty = FALSE;
	}else{
		t->file->mod = TRUE;
		dirty = TRUE;
	}
	for(i=0; i<t->file->ntext; i++)
		t->file->text[i]->w->dirty = dirty;
	free(name);
	free(r);
	w->settag();
	for(i=0; i<t->file->ntext; i++){
		u = t->file->text[i];
		u->w->tag.setselect(u->w->tag.file->nc, u->w->tag.file->nc);
		u->scrdraw();
	}
}

void
put(Text *et, Text *t, Text *argt, int, int, Rune *arg, int narg)
{
	uint q0, q1, n, m, nname;
	Rune *r, *namer;
	byte *s, *name;
	Window *w;
	int i, fd;
	Dir d;

	if(et==nil || et->w==nil || et->w->isdir)
		return;
	w = et->w;
	t = &w->body;
	rescue{
		free(name);
		free(namer);
		return;
	}
	name = getname(t, argt, arg, narg, TRUE);
	if(name == nil){
		warning(nil, "no file name\n");
		return;
	}
	(namer, nname) = bytetorune(name);
	fd = create(name, OWRITE, 0666);
	if(fd < 0){
		warning(nil, "can't create file %s: %r\n", name);
		raise;
	}
	r = fbufalloc();
	s = fbufalloc();
	rescue{
		fbuffree(s);
		fbuffree(r);
		close(fd);
		raise;
	}
	if(dirfstat(fd, &d)>=0 && (d.mode&CHAPPEND) && d.length>0){
		warning(nil, "%s not written; file is append only\n", name);
		raise;
	}
	q0 = 0;
	q1 = t->file->nc;
	while(q0 < q1){
		n = q1 - q0;
		if(n > BUFSIZE/UTFmax)
			n = BUFSIZE/UTFmax;
		t->file->read(q0, r, n);
		m = snprint(s, BUFSIZE+1, "%.*S", n, r);
		if(write(fd, s, m) != m){
			warning(nil, "can't write file %s: %r\n", name);
			raise;
		}
		q0 += n;
	}
	if(runeeq(namer, nname, t->file->name, t->file->nname)){
		t->file->mod = FALSE;
		w->dirty = FALSE;
	}
	for(i=0; i<t->file->ntext; i++){
		t->file->text[i]->w->putseq = t->file->seq;
		t->file->text[i]->w->dirty = w->dirty;
	}
	fbuffree(s);
	fbuffree(r);
	free(name);
	free(namer);
	close(fd);
	w->settag();
}

void
dump(Text *, Text *, Text *argt, int isdump, int, Rune *arg, int narg)
{
	byte *name;

	if(narg)
		name = runetobyte(arg, narg);
	else
		(nil, name) = getbytearg(argt, FALSE, TRUE);
	if(isdump)
		row.dump(name);
	else
		row.load(name, FALSE);
	free(name);
}

void
cut(Text *et, Text *t, Text*, int dosnarf, int docut, Rune*, int)
{
	uint q0, q1, n, locked, c;
	Rune *r;

	if(t == nil)
		return;
	/* use current window if snarfing and its selection is non-null */
	if(et!=t && dosnarf && et->w!=nil){
		if(et->w->body.q1>et->w->body.q0)
			t = &et->w->body;
		else if(et->w->tag.q1>et->w->tag.q0)
			t = &et->w->tag;
	}
	locked = FALSE;
	if(t->w!=nil && et->w!=t->w){
		locked = TRUE;
		c = 'M';
		if(et->w)
			c = et->w->owner;
		t->w->lock(c);
	}
	if(t->q0 == t->q1){
		if(locked)
			t->w->unlock();
		return;
	}
	if(dosnarf){
		q0 = t->q0;
		q1 = t->q1;
		snarfbuf.delete(0, snarfbuf.nc);
		r = fbufalloc();
		while(q0 < q1){
			n = q1 - q0;
			if(n > RBUFSIZE)
				n = RBUFSIZE;
			t->file->read(q0, r, n);
			snarfbuf.insert(snarfbuf.nc, r, n);
			q0 += n;
		}
		fbuffree(r);
	}
	if(docut){
		t->delete(t->q0, t->q1, TRUE);
		t->setselect(t->q0, t->q0);
		if(t->w)
			t->w->settag();
	}else if(dosnarf)	/* Snarf command */
		argtext = t;
	if(locked)
		t->w->unlock();
}

void
paste(Text *et, Text *t, Text*, int flag1, int, Rune*, int)
{
	int c;
	uint q, q0, q1, n;
	Rune *r;

	if(t==nil || snarfbuf.nc==0)
		return;
	if(t->w!=nil && et->w!=t->w){
		c = 'M';
		if(et->w)
			c = et->w->owner;
		t->w->lock(c);
	}
	cut(t, t, nil, FALSE, TRUE, nil, 0);
	q = 0;
	q0 = t->q0;
	q1 = t->q0+snarfbuf.nc;
	r = fbufalloc();
	while(q0 < q1){
		n = q1 - q0;
		if(n > RBUFSIZE)
			n = RBUFSIZE;
		if(r == nil)
			r = runemalloc(n);
		snarfbuf.read(q, r, n);
		t->insert(q0, r, n, TRUE);
		q += n;
		q0 += n;
	}
	fbuffree(r);
	if(flag1)
		t->setselect(t->q0, q1);
	else
		t->setselect(q1, q1);
	if(t->w)
		t->w->settag();
	if(t->w!=nil && et->w!=t->w)
		t->w->unlock();
}

void
look(Text *et, Text *t, Text *argt, int, int, Rune*, int)
{
	Rune *r;
	int n;

	if(et && et->w){
		t = &et->w->body;
		(nil, r, n) = getarg(argt, FALSE, FALSE);
		if(r == nil){
			n = t->q1-t->q0;
			r = runemalloc(n);
			t->file->read(t->q0, r, n);
		}
		search(t, r, n);
		free(r);
	}
}

void
send(Text *et, Text *t, Text*, int, int, Rune*, int)
{
	if(et->w==nil)
		return;
	t = &et->w->body;
	if(t->q0 != t->q1)
		cut(t, t, nil, TRUE, FALSE, nil, 0);
	t->setselect(t->file->nc, t->file->nc);
	paste(t, t, nil, FALSE, XXX, nil, 0);
	if(t->readc(t->file->nc-1) != '\n'){
		t->insert(t->file->nc, $"\n", 1, TRUE);
		t->setselect(t->file->nc, t->file->nc);
	}
}

void
exit(Text*, Text*, Text*, int, int, Rune*, int)
{
	if(row.clean()){
		*mouseexit1 <-= 1;
		<-*mouseexit0;
		terminate(nil);
	}
}

void
putall(Text*, Text*, Text*, int, int, Rune*, int)
{
	int i, j, e;
	Window *w;
	Column *c;
	byte *a;

	for(i=0; i<row.ncol; i++){
		c = row.col[i];
		for(j=0; j<c->nw; j++){
			w = c->w[j];
			if(w->isscratch || w->isdir || w->body.file->nname==0)
				continue;
			if(w->nopen[QWevent] > 0)
				continue;
			a = runetobyte(w->body.file->name, w->body.file->nname);
			e = access(a, 0);
			if(w->body.file->mod || w->body.ncache)
				if(e < 0)
					warning(nil, "no auto-Put of %s: %r\n", a);
				else{
					w->commit(&w->body);
					put(&w->body, nil, nil, XXX, XXX, nil, 0);
				}
			free(a);
		}
	}
}


void
id(Text *et, Text*, Text*, int, int, Rune*, int)
{
	if(et && et->w)
		warning(nil, "/mnt/acme/%d/\n", et->w->id);
}

void
local(Text *et, Text*, Text *argt, int, int, Rune *arg, int narg)
{
	byte *a, *aa;
	Rune *dir;
	int n;

	(aa, a) = getbytearg(argt, TRUE, TRUE);

	(dir, n) = dirname(et, nil, 0);
	if(n==1 && dir[0]=='.'){	/* sigh */
		free(dir);
		dir = nil;
		n = 0;
	}
	proc run(nil, runetobyte(arg, narg), dir, n, FALSE, aa, a);
}

void
kill(Text*, Text*, Text *argt, int, int, Rune *arg, int narg)
{
	Rune *a, *cmd, *r;
	int na;

	(nil, r, na) = getarg(argt, FALSE, FALSE);
	if(r)
		kill(nil, nil, nil, 0, 0, r, na);
	/* loop condition: *arg is not a blank */
	for(;;){
		(a, na) = findbl(arg, narg);
		if(a == arg)
			break;
		cmd = runemalloc(narg-na+1);
		runemove(cmd, arg, narg-na);
		ckill <-= cmd;
		(arg, narg) = skipbl(a, na);
	}
}

void
fontx(Text *et, Text *t, Text *argt, int, int, Rune *arg, int narg)
{
	Rune *a, *r, *flag, *file;
	int na, nf;
	byte *aa;
	Reffont *newfont;
	Dirlist *dp;
	int i, fix;

	if(et==nil || et->w==nil)
		return;
	t = &et->w->body;
	flag = nil;
	file = nil;
	/* loop condition: *arg is not a blank */
	nf = 0;
	for(;;){
		(a, na) = findbl(arg, narg);
		if(a == arg)
			break;
		r = runemalloc(narg-na+1);
		runemove(r, arg, narg-na);
		if(runeeq(r, narg-na, $"fix", 3) || runeeq(r, narg-na, $"var", 3)){
			free(flag);
			flag = r;
		}else{
			free(file);
			file = r;
			nf = narg-na;
		}
		(arg, narg) = skipbl(a, na);
	}
	(nil, r, na) = getarg(argt, FALSE, TRUE);
	if(r)
		if(runeeq(r, na, $"fix", 3) || runeeq(r, na, $"var", 3)){
			free(flag);
			flag = r;
		}else{
			free(file);
			file = r;
			nf = na;
		}
	fix = 1;
	if(flag)
		fix = runeeq(flag, runestrlen(flag), $"fix", 3);
	else if(file == nil){
		newfont = .Reffont.get(FALSE, FALSE, FALSE, nil);
		if(newfont)
			fix = strcmp(newfont->f->name, t->font->name)==0;
	}
	if(file){
		aa = runetobyte(file, nf);
		newfont = .Reffont.get(fix, flag!=nil, FALSE, aa);
		free(aa);
	}else
		newfont = .Reffont.get(fix, FALSE, FALSE, nil);
	if(newfont){
		bitblt(&screen, t->w->r.min, &screen, t->w->r, 0);
		t->reffont->close();
		t->reffont = newfont;
		t->font = newfont->f;
		if(t->w->isdir){
			t->all.min.x++;	/* force recolumnation; disgusting! */
			for(i=0; i<t->w->ndl; i++){
				dp = t->w->dlp[i];
				aa = runetobyte(dp->r, dp->nr);
				dp->wid = strwidth(newfont->f, aa);
				free(aa);
			}
		}
		/* avoid shrinking of window due to quantization */
		t->w->col->grow(t->w, -1);
	}
	free(file);
	free(flag);
}

void
incl(Text *et, Text*, Text *argt, int, int, Rune *arg, int narg)
{
	Rune *a, *r;
	Window *w;
	int na, n, len;

	if(et==nil || et->w==nil)
		return;
	w = et->w;
	n = 0;
	(nil, r, len) = getarg(argt, FALSE, TRUE);
	if(r){
		n++;
		w->addincl(r, len);
	}
	/* loop condition: *arg is not a blank */
	for(;;){
		(a, na) = findbl(arg, narg);
		if(a == arg)
			break;
		r = runemalloc(narg-na+1);
		runemove(r, arg, narg-na);
		n++;
		w->addincl(r, narg-na);
		(arg, narg) = skipbl(a, na);
	}
	if(n==0 && w->nincl){
		for(n=w->nincl; --n>=0; )
			warning(nil, "%S ", w->incl[n]);
		warning(nil, "\n");
	}
}

void
runfeed(int *p, chan(int) c)
{
	byte *buf;

	rfork(RFFDG);
	c <-= 1;
	close(p[1]);
	for(;;){
		buf = malloc(257);
		if(read(p[0], buf, 256) <= 0)
			break;
		cerr <-= buf;
	}
	terminate(nil);
}

void
run(Window *win, byte *s, Rune *rdir, int ndir, int newns, byte *argaddr, byte *arg)
{
	Command *c;
	byte *e, *t, *name, *dir, **av, *news;
	Rune r, **incl;
	int ac, w, inarg, i, n, fd, p[2], nincl;
	chan(int) pc;
	byte buf[512];

	c = malloc(sizeof *c);
	t = s;
	while(*t==' ' || *t=='\n' || *t=='\t')
		t++;
	for(e=t; *e; e++)
		if(*e==' ' || *e=='\n' || *e=='\t' )
			break;
	name = malloc((e-t)+2);
	memmove(name, t, e-t);
	name[e-t] = 0;
	e = utfrrune(name, '/');
	if(e)
		strcpy(name, e+1);
	strcat(name, " ");	/* add blank here for ease in waittask */
	(c->name, c->nname) = bytetorune(name);
	free(name);
	c->pid = getpid();
	c->text = s;
	ccommand <-= c;
	if(rdir != nil){
		dir = runetobyte(rdir, ndir);
		chdir(dir);	/* ignore error: probably app. window */
		free(dir);
	}
	/*
	 * must rfork() after communication because rendezvous name
	 * space is part of RFNAMEG.
	 */
	if(newns){
		if(win){
			nincl = win->nincl;
			incl = malloc(nincl*sizeof(Rune*));
			for(i=0; i<nincl; i++){
				n = runestrlen(win->incl[i]);
				incl[i] = runemalloc(n+1);
				runemove(incl[i], win->incl[i], n);
			}
			win->close();
		}else{
			nincl = 0;
			incl = nil;
		}
		rfork(RFNAMEG|RFENVG|RFFDG|RFNOTEG);
		close(bitbltfd);
		c->md = fsysmount(rdir, ndir, incl, nincl);
		if(c->md == nil){
			fprint(2, "child: can't mount /dev/cons: %r\n");
			terminate("mount");
		}
		close(0);
		open("/dev/null", OREAD);
		close(1);
		open("/dev/cons", OWRITE);
		dup(1, 2);
	}else{
		if(win)
			win->close();
		rfork(RFFDG|RFNOTEG);
		if(pipe(p) < 0){
			fprint(2, "child: can't pipe: %r\n");
			terminate("pipe");
		}
		alloc pc;
		proc runfeed(p, pc);
		<-pc;
		unalloc(pc);
		close(bitbltfd);
		fsysclose();
		close(0);
		open("/dev/null", OREAD);
		dup(p[1], 1);
		dup(1, 2);
		close(p[1]);
		close(p[0]);
	}

	if(argaddr)
		setenv("acmeaddr", argaddr);
	if(strlen(s) > sizeof buf-10)	/* may need to print into stack */
		goto Hard;
	inarg = FALSE;
	for(e=s; *e; e+=w){
		w = chartorune(&r, e);
		if(r==' ' || r=='\t')
			continue;
		if(r < ' ')
			goto Hard;
		if(utfrune("#;&|^$=`'{}()<>[]*?^~`", r))
			goto Hard;
		inarg = TRUE;
	}
	if(!inarg)
		terminate(nil);
	ac = 0;
	av = nil;
	inarg = FALSE;
	for(e=s; *e; e+=w){
		w = chartorune(&r, e);
		if(r==' ' || r=='\t'){
			inarg = FALSE;
			*e = 0;
			continue;
		}
		if(!inarg){
			inarg = TRUE;
			av = realloc(av, (ac+1)*sizeof(byte**));
			av[ac++] = e;
		}
	}
	av = realloc(av, (ac+2)*sizeof(byte**));
	av[ac++] = arg;
	av[ac] = nil;
	c->av = av;
	exec(av[0], av);
	e = av[0];
	if(e[0]=='/' || (e[0]=='.' && e[1]=='/'))
		goto Fail;
	if(cputype){
		sprint(buf, "%s/%s", cputype, av[0]);
		exec(buf, av);
	}
	sprint(buf, "/bin/%s", av[0]);
	exec(buf, av);
	goto Fail;

Hard:

	/*
	 * ugly: set path = (. $cputype /bin)
	 * should honor $path if unusual.
	 * see also exec.l:run()
	 */
	if(cputype){
		n = 0;
		memmove(buf+n, ".", 2);
		n += 2;
		i = strlen(cputype)+1;
		memmove(buf+n, cputype, i);
		n += i;
		memmove(buf+n, "/bin", 5);
		n += 5;
		fd = create("/env/path", OWRITE, 0666);
		write(fd, buf, n);
		close(fd);
	}

	if(arg){
		news = malloc(strlen(s) + 1 + 1 + strlen(arg) + 1 + 1);
		if(news){
			sprint(news, "%s '%s'", s, arg);	/* BUG: what if quote in arg? */
			free(s);
			s = news;
			c->text = news;
		}
	}
	execl("/bin/rc", "rc", "-c", s, nil);

   Fail:
	terminate("can't exec");
}
