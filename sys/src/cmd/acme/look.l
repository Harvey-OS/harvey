#include <alef.h>
#include <libg.h>
#include <frame.h>
#include <fcall.h>
#include <regexp.h>
#include "dat.h"
#include "fns.h"

Window*	openfile(Text*, Expand*);

void
look3(Text *t, uint q0, uint q1, int external)
{
	int n, c, f;
	Text *ct;
	Expand e;
	Rune *r;

	ct = seltext;
	if(ct == nil)
		seltext = t;
	if(expand(t, q0, q1, &e) == FALSE)
		return;
	n = e.q1 - e.q0;
	if(!external && t->w!=nil && t->w->nopen[QWevent]>0){
		f = 0;
		if((e.at!=nil && t->w!=nil) || (e.nname>0 && lookfile(e.name, e.nname)!=nil))
			f = 1;		/* acme can do it without loading a file */
		if(q0!=e.q0 || q1!=e.q1)
			f |= 2;	/* second (post-expand) message follows */
		if(e.nname)
			f |= 4;	/* it's a file name */
		c = 'l';
		if(t->what == Body)
			c = 'L';
		n = q1-q0;
		if(n <= EVENTSIZE){
			r = runemalloc(n);
			t->file->read(q0, r, n);
			t->w->event("%c%d %d %d %d %.*S\n", c, q0, q1, f, n, n, r);
			free(r);
		}else
			t->w->event("%c%d %d %d 0 \n", c, q0, q1, f, n);
		if(q0==e.q0 && q1==e.q1)
			return;
		if(e.nname){
			n = e.nname;
			if(e.a1 > e.a0)
				n += 1+(e.a1-e.a0);
			r = runemalloc(n);
			runemove(r, e.name, e.nname);
			if(e.a1 > e.a0){
				r[e.nname] = ':';
				e.at->file->read(e.a0, r+e.nname+1, e.a1-e.a0);
			}
		}else{
			n = e.q1 - e.q0;
			r = runemalloc(n);
			t->file->read(e.q0, r, n);
		}
		f &= ~2;
		if(n <= EVENTSIZE)
			t->w->event("%c%d %d %d %d %.*S\n", c, e.q0, e.q1, f, n, n, r);
		else
			t->w->event("%c%d %d %d 0 \n", c, e.q0, e.q1, f, n);
		free(r);
		return;
	}
	if(e.name || e.at)
		openfile(t, &e);
	else{
		if(t->w == nil)
			return;
		ct = &t->w->body;
		if(t->w != ct->w)
			ct->w->lock('M');
		if(t == ct)
			ct->setselect(e.q1, e.q1);
		r = runemalloc(n);
		t->file->read(e.q0, r, n);
		if(search(ct, r, n) && e.jump)
			cursorset(add(frptofchar(ct, ct->p0), Pt(4, ct->font->height-4)));
		if(t->w != ct->w)
			ct->w->unlock();
		free(r);
	}
	free(e.name);
	free(e.bname);
}

int
search(Text *ct, Rune *r, uint n)
{
	uint q, nb, maxn;
	int around;
	Rune *s, *b, *c;

	if(n==0 || n>ct->file->nc)
		return FALSE;
	if(2*n > RBUFSIZE){
		warning(nil, "string too long\n");
		return FALSE;
	}
	maxn = max(2*n, RBUFSIZE);
	s = fbufalloc();
	b = s;
	nb = 0;
	b[nb] = 0;
	around = 0;
	q = ct->q1;
	for(;;){
		if(q >= ct->file->nc){
			q = 0;
			around = 1;
			nb = 0;
			b[nb] = 0;
		}
		if(nb > 0){
			c = strrune(b, r[0]);
			if(c == nil){
				q += nb;
				nb = 0;
				b[nb] = 0;
				if(around && q>=ct->q1)
					break;
				continue;
			}
			q += (c-b);
			nb -= (c-b);
			b = c;
		}
		/* reload if buffer covers neither string nor rest of file */
		if(nb<n && nb!=ct->file->nc-q){
			nb = ct->file->nc-q;
			if(nb > maxn)
				nb = maxn;
			ct->file->read(q, s, nb);
			b = s;
			b[nb] = 0;
		}
		/* this runeeq is fishy but the null at b[nb] makes it safe */
		if(runeeq(b, n, r, n)==TRUE){
			if(ct->w){
				ct->show(q, q+n);
				ct->w->settag();
			}else{
				ct->q0 = q;
				ct->q1 = q+n;
			}
			seltext = ct;
			fbuffree(s);
			return TRUE;
		}
		if(around && q>=ct->q1)
			break;
		--nb;
		b++;
		q++;
	}
	fbuffree(s);
	return FALSE;
}

int
isfilec(Rune r)
{
	if(isalnum(r))
		return TRUE;
	if(strrune($".-+/:", r))
		return TRUE;
	return FALSE;
}

(Rune*, int)
cleanname(Rune *b, int n)
{
	int i, j, found;

	/* compress multiple slashes */
	for(i=0; i<n-1; i++)
		if(b[i]=='/' && b[i+1]=='/'){
			runemove(b+i, b+i+1, n-i-1);
			--n;
			--i;
		}
	/*  eliminate ./ */
	for(i=0; i<n-1; i++)
		if(b[i]=='.' && b[i+1]=='/' && (i==0 || b[i-1]=='/')){
			runemove(b+i, b+i+2, n-i-2);
			n -= 2;
			--i;
		}
	/* eliminate trailing . */
	if(n>=2 && b[n-2]=='/' && b[n-1]=='.')
		--n;
	do{
		/* compress xx/.. */
		found = FALSE;
		for(i=1; i<=n-3; i++)
			if(runeeq(b+i, 3, $"/..", 3)){
				if(i==n-3 || b[i+3]=='/'){
					found = TRUE;
					break;
				}
			}
		if(found)
			for(j=i-1; j>=0; --j)
				if(j==0 || b[j-1]=='/'){
					i += 3;		/* character beyond .. */
					if(i<n && b[i]=='/')
						++i;
					runemove(b+j, b+i, n-i);
					n -= (i-j);
					break;
				}
	}while(found);
	if(n == 0){
		*b = '.';
		n = 1;
	}
	return (b, n);
}

(Rune*, int)
includefile(Rune *dir, Rune *file, int nfile)
{
	int m, n;
	byte *a;
	Rune *r;

	m = runestrlen(dir);
	a = malloc((m+1+nfile)*UTFmax+1);
	sprint(a, "%S/%.*S", dir, nfile, file);
	n = access(a, 0);
	free(a);
	if(n < 0)
		return (nil, 0);
	r = runemalloc(m+1+nfile);
	runemove(r, dir, m);
	runemove(r+m, $"/", 1);
	runemove(r+m+1, file, nfile);
	free(file);
	return cleanname(r, m+1+nfile);
}

intern	Rune	*objdir;
intern	Rune	*alefobjdir;

(Rune*, int)
includename(Text *t, Rune *r, int n)
{
	Rune *file;
	int i, nfile;
	Window *w;
	byte buf[128];

	if(objdir==nil && objtype!=nil){
		sprint(buf, "/%s/include", objtype);
		(objdir, i) = bytetorune(buf);
		objdir = runerealloc(objdir, i+1);
		objdir[i] = 0;					/* null terminate */
		sprint(buf, "/%s/include/alef", objtype);
		(alefobjdir, i) = bytetorune(buf);
		alefobjdir = runerealloc(alefobjdir, i+1);
		alefobjdir[i] = 0;				/* null terminate */
	}

	rescue{
		return (r, n);
	}
	w = t->w;
	if(n==0 || r[0]=='/' || w==nil)
		raise;
	if(n>2 && r[0]=='.' && r[1]=='/')
		raise;
	file = nil;
	nfile = 0;
	for(i=0; i<w->nincl && file==nil; i++)
		(file, nfile) = includefile(w->incl[i], r, n);
	if(file == nil && t->file->nname>2 && runeeq(t->file->name+t->file->nname-2, 2, $".l", 2)){
		(file, nfile) = includefile($"/sys/include/alef/", r, n);
		if(file==nil && alefobjdir!=nil)
			(file, nfile) = includefile(alefobjdir, r, n);
	}else{
		if(file == nil)
			(file, nfile) = includefile($"/sys/include", r, n);
		if(file==nil && objdir!=nil)
			(file, nfile) = includefile(objdir, r, n);
	}
	if(file == nil)
		raise;
	return (file, nfile);
}

(Rune*, int)
dirname(Text *t, Rune *r, int n)
{
	Rune *b, c;
	uint m, nt;
	int slash;

	rescue{
		free(b);
		if(r)
			return cleanname(r, n);
		return (r, n);
	}
	b = nil;
	if(t->w == nil)
		raise;
	nt = t->w->tag.file->nc;
	if(nt == 0)
		raise;
	if(n>=1 &&  r[0]=='/')
		raise;
	b = runemalloc(nt+n+1);
	t->w->tag.file->read(0, b, nt);
	slash = -1;
	for(m=0; m<nt; m++){
		c = b[m];
		if(c == '/')
			slash = m;
		if(c==' ' || c=='\t')
			break;
	}
	if(slash < 0)
		raise;
	runemove(b+slash+1, r, n);
	free(r);
	return cleanname(b, slash+1+n);
}

int
expandfile(Text *t, uint q0, uint q1, Expand *e)
{
	int i, n, nname, colon;
	uint amin, amax;
	Rune *r, c;
	Window *w;

	amax = q1;
	if(q1 == q0){
		colon = -1;
		while(q1<t->file->nc && isfilec(c=t->readc(q1))){
			if(c == ':'){
				colon = q1;
				break;
			}
			q1++;
		}
		while(q0>0 && isfilec(c=t->readc(q0-1))){
			q0--;
			if(colon==-1 && c==':')
				colon = q0;
		}
		/*
		 * if it looks like it might begin file: , consume address chars after :
		 * otherwise terminate expansion at :
		 */
		if(colon>=0 && colon<t->file->nc && isaddrc(t->readc(colon+1))){
			q1 = colon+1;
			while(q1<t->file->nc && isaddrc(t->readc(q1)))
				q1++;
		}else if(colon >= 0)
			q1 = colon;
		if(q1 > q0)
			amax = t->file->nc;
	}
	amin = amax;
	e->q0 = q0;
	e->q1 = q1;
	n = q1-q0;
	if(n == 0)
		return FALSE;
	/* see if it's a file name */
	r = runemalloc(n);
	t->file->read(q0, r, n);
	/* first, does it have bad chars? */
	nname = -1;
	for(i=0; i<n; i++){
		c = r[i];
		if(c==':' && nname<0){
			if(q0+i+1<t->file->nc && (i==n-1 || isaddrc(t->readc(q0+i+1))))
				amin = q0+i;
			else
				goto Isntfile;
			nname = i;
		}
	}
	if(nname == -1)
		nname = n;
	for(i=0; i<nname; i++)
		if(!isfilec(r[i]))
			goto Isntfile;
	/*
	 * See if it's a file name in <>, and turn that into an include
	 * file name if so.  Should probably do it for "" too, but that's not
	 * restrictive enough syntax and checking for a #include earlier on the
	 * line would be silly.
	 */
	if(q0>0 && t->readc(q0-1)=='<' && q1<t->file->nc && t->readc(q1)=='>')
		(r, nname) = includename(t, r, nname);
	else if(amin == q0)
		goto Isfile;
	else
		(r, nname) = dirname(t, r, nname);
	e->bname = runetobyte(r, nname);
	/* if it's already a window name, it's a file */
	w = lookfile(r, nname);
	if(w != nil)
		goto Isfile;
	/* if it's the name of a file, it's a file */
	if(access(e->bname, 0) < 0){
		free(e->bname);
		e->bname = nil;
		goto Isntfile;
	}

  Isfile:
	e->name = r;
	e->nname = nname;
	e->at = t;
	e->a0 = amin+1;
	(nil, e->a1, nil) = address(nil, (Range)(-1,-1), (Range)(0, 0), t, e->a0, amax, tgetc, FALSE);
	return TRUE;

   Isntfile:
	free(r);
	return FALSE;
}

int
expand(Text *t, uint q0, uint q1, Expand *e)
{
	memset(e, 0, sizeof *e);
	/* if in selection, choose selection */
	e->jump = TRUE;
	if(q1==q0 && t->q1>t->q0 && t->q0<=q0 && q0<=t->q1){
		q0 = t->q0;
		q1 = t->q1;
		if(t->what == Tag)
			e->jump = FALSE;
	}

	if(expandfile(t, q0, q1, e))
		return TRUE;

	if(q0 == q1){
		while(q1<t->file->nc && isalnum(t->readc(q1)))
			q1++;
		while(q0>0 && isalnum(t->readc(q0-1)))
			q0--;
	}
	e->q0 = q0;
	e->q1 = q1;
	return q1 > q0;
}

Window*
lookfile(Rune *s, int n)
{
	int i, j, k;
	Window *w;
	Column *c;
	Text *t;

	/* avoid terminal slash on directories */
	if(s[n-1] == '/')
		--n;
	for(j=0; j<row.ncol; j++){
		c = row.col[j];
		for(i=0; i<c->nw; i++){
			w = c->w[i];
			t = &w->body;
			k = t->file->nname;
			if(k>0 && t->file->name[k-1] == '/')
				k--;
			if(runeeq(t->file->name, k, s, n))
				return w->body.file->curtext->w;
		}
	}
	return nil;
}

Window*
lookid(int id, int dump)
{
	int i, j;
	Window *w;
	Column *c;

	for(j=0; j<row.ncol; j++){
		c = row.col[j];
		for(i=0; i<c->nw; i++){
			w = c->w[i];
			if(dump && w->dumpid == id)
				return w;
			if(!dump && w->id == id)
				return w;
		}
	}
	return nil;
}


Window*
openfile(Text *t, Expand *e)
{
	Range r;
	Window *w, *ow;
	int eval, i, n;
	Rune *rp;

	if(e->nname == 0){
		w = t->w;
		if(w == nil)
			return nil;
	}else
		w = lookfile(e->name, e->nname);
	if(w)
		t = &w->body;
	else{
		ow = t->w;
		w = newwindow(t);
		t = &w->body;
		w->setname(e->name, e->nname);
		t->load(0, e->bname);
		t->file->mod = FALSE;
		t->w->dirty = FALSE;
		t->w->settag();
		t->w->tag.setselect(t->w->tag.file->nc, t->w->tag.file->nc);
		if(ow != nil)
			for(i=ow->nincl; --i>=0; ){
				n = runestrlen(ow->incl[i]);
				rp = runemalloc(n);
				runemove(rp, ow->incl[i], n);
				w->addincl(rp, n);
			}
	}
	if(e->a1 == e->a0)
		eval = FALSE;
	else
		(eval, nil, r) = address(t, (Range)(-1, -1), (Range)(t->q0, t->q1), e->at, e->a0, e->a1, tgetc, TRUE);
	if(eval == FALSE){
		r.q0 = t->q0;
		r.q1 = t->q1;
	}
	t->show(r.q0, r.q1);
	t->w->settag();
	seltext = t;
	if(e->jump)
		cursorset(add(frptofchar(t, t->p0), Pt(4, t->font->height-4)));
	return w;
}

void
new(Text *et, Text *t, Text *argt, int flag1, int flag2, Rune *arg, int narg)
{
	int ndone;
	Rune *a, *f;
	int na, nf;
	Expand e;

	(nil, a, na) = getarg(argt, FALSE, TRUE);
	if(a){
		new(et, t, nil, flag1, flag2, a, na);
		if(narg == 0)
			return;
	}
	/* loop condition: *arg is not a blank */
	for(ndone=0; ; ndone++){
		(a, na) = findbl(arg, narg);
		if(a == arg){
			if(ndone==0 && et->col!=nil)
				et->col->add(nil, nil, -1)->settag();
			break;
		}
		nf = narg-na;
		f = runemalloc(nf);
		runemove(f, arg, nf);
		(f, nf) = dirname(et, f, nf);
		memset(&e, 0, sizeof e);
		e.name = f;
		e.nname = nf;
		e.bname = runetobyte(f, nf);
		e.jump = TRUE;
		openfile(et, &e);
		free(f);
		free(e.bname);
		(arg, narg) = skipbl(a, na);
	}
}
