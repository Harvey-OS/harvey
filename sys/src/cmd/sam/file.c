#include "sam.h"
/*
 * Files are splayed out a factor of NDISC to reduce indirect block access
 */
Discdesc	*files[NDISC];
Discdesc	*transcripts[NDISC];
Buffer		*undobuf;
static String	*ftempstr(Rune*, int);
int		fcount;
File		*lastfile;

void	puthdr_csl(Buffer*, char, short, Posn);
void	puthdr_cs(Buffer*, char, short);
void	puthdr_M(Buffer*, Posn, Range, Range, Mod, short);
void	puthdr_cll(Buffer*, char, Posn, Posn);
void	Fflush(File*);

enum{
	SKIP=50,		/* max dist between file changes folded together */
	MAXCACHE=STRSIZE,	/* max length of cache. must be < 32K-BLOCKSIZE */
};

void
Fstart(void)
{
	undobuf = Bopen(Dstart());
	snarfbuf = Bopen(Dstart());
	plan9buf = Bopen(Dstart());
}

void
Fmark(File *f, Mod m)
{
	Buffer *t = f->transcript;
	Posn p;

	if(f->state == Readerr)
		return;
	if(f->state == Unread)	/* this is implicit 'e' of a file */
		return;
	p = m==0? -1 : f->markp;
	f->markp = t->nrunes;
	puthdr_M(t, p, f->dot.r, f->mark, f->mod, f->state);
	f->ndot = f->dot;
	f->marked = TRUE;
	f->mod = m;
	f->hiposn = -1;
	/* Safety first */
	f->cp1 = f->cp2 = 0;
}

File *
Fopen(void)
{
	File *f;

	f = emalloc(sizeof(File));
	if(files[fcount] == 0){
		files[fcount] = Dstart();
		transcripts[fcount] = Dstart();
	}
	f->buf = Bopen(files[fcount]);
	f->transcript = Bopen(transcripts[fcount]);
	if(++fcount == NDISC)
		fcount = 0;
	f->nrunes = 0;
	f->markp = 0;
	f->mod = 0;
	f->dot.f = f;
	f->ndot.f = f;
	f->dev = ~0;
	f->qid = ~0;
	Strinit0(&f->name);
	Strinit(&f->cache);
	f->state = Unread;
	Fmark(f, (Mod)0);
	return f;
}

void
Fclose(File *f)
{
	if(f == lastfile)
		lastfile = 0;
	Bclose(f->buf);
	Bclose(f->transcript);
	Strclose(&f->name);
	Strclose(&f->cache);
	if(f->rasp)
		listfree(f->rasp);
	free(f);
}

void
Finsert(File *f, String *str, Posn p1)
{
	Buffer *t = f->transcript;

	if(f->state == Readerr)
		return;
	if(str->n == 0)
		return;
	if(str->n<0 || str->n>STRSIZE)
		panic("Finsert");
	if(f->mod < modnum)
		Fmark(f, modnum);
	if(p1 < f->hiposn)
		error(Esequence);
	if(str->n >= BLOCKSIZE){	/* don't bother with the cache */
		Fflush(f);
		puthdr_csl(t, 'i', str->n, p1);
		Binsert(t, str, t->nrunes);
	}else{	/* insert into the cache instead of the transcript */
		if(f->cp2==0 && f->cp1==0 && f->cache.n==0)	/* empty cache */
			f->cp1 = f->cp2 = p1;
		if(p1-f->cp2>SKIP || f->cache.n+str->n>MAXCACHE-SKIP){
			Fflush(f);
			f->cp1 = f->cp2 = p1;
		}
		if(f->cp2 != p1){	/* grab the piece in between */
			Rune buf[SKIP];
			String s;
			Fchars(f, buf, f->cp2, p1);
			s.s = buf;
			s.n = p1-f->cp2;
			Strinsert(&f->cache, &s, f->cache.n);
			f->cp2 = p1;
		}
		Strinsert(&f->cache, str, f->cache.n);
	}
	if(f != cmd)
		quitok = FALSE;
	f->closeok = FALSE;
	if(f->state == Clean)
		state(f, Dirty);
	f->hiposn = p1;
}

void
Fdelete(File *f, Posn p1, Posn p2)
{
	if(f->state == Readerr)
		return;
	if(p1==p2)
		return;
	if(f->mod<modnum)
		Fmark(f, modnum);
	if(p1<f->hiposn)
		error(Esequence);
	if(p1-f->cp2>SKIP)
		Fflush(f);
	if(f->cp2==0 && f->cp1==0 && f->cache.n==0)	/* empty cache */
		f->cp1 = f->cp2 = p1;
	if(f->cp2 != p1){	/* grab the piece in between */
		if(f->cache.n+(p1-f->cp2)>MAXCACHE){
			Fflush(f);
			f->cp1 = f->cp2 = p1;
		}else{
			Rune buf[SKIP];
			String s;
			Fchars(f, buf, f->cp2, p1);
			s.s = buf;
			s.n = p1-f->cp2;
			Strinsert(&f->cache, &s, f->cache.n);
		}
	}
	f->cp2 = p2;
	if(f!=cmd)
		quitok = FALSE;
	f->closeok = FALSE;
	if(f->state==Clean)
		state(f, Dirty);
	f->hiposn = p2;
}

void
Fflush(File *f)
{
	Buffer *t = f->transcript;
	Posn p1 = f->cp1, p2 = f->cp2;

	if(f->state == Readerr)
		return;
	if(p1 != p2)
		puthdr_cll(t, 'd', p1, p2);
	if(f->cache.n){
		puthdr_csl(t, 'i', f->cache.n, p2);
		Binsert(t, &f->cache, t->nrunes);
		Strzero(&f->cache);
	}
	f->cp1 = f->cp2 = 0;
}

void
Fsetname(File *f, String *s)
{
	Buffer *t = f->transcript;

	if(f->state == Readerr)
		return;
	if(f->state == Unread){	/* This is setting initial file name */
		Strduplstr(&f->name, s);
		sortname(f);
	}else{
		if(f->mod < modnum)
			Fmark(f, modnum);
		puthdr_cs(t, 'f', s->n);
		Binsert(t, s, t->nrunes);
	}
}

/*
 * The heart of it all. Fupdate will run along the transcript list, executing
 * the commands and converting them into their inverses for a later undo pass.
 * The pass runs top to bottom, so addresses in the transcript are tracked
 * (by the var. delta) so they stay valid during the operation.  This causes
 * all operations to appear to happen simultaneously, which is why the addresses
 * passed to Fdelete and Finsert never take into account other changes occurring
 * in this command (and is why things are done this way).
 */
int
Fupdate(File *f, int mktrans, int toterm)
{
	Buffer *t = f->transcript;
	Buffer *u = undobuf;
	int n, ni;
	Posn p0, p1, p2, p, deltadot = 0, deltamark = 0, delta = 0;
	int changes = FALSE;
	union Hdr buf;
	Rune tmp[BLOCKSIZE+1];	/* +1 for NUL in 'f' case */

	if(f->state == Readerr)
		return FALSE;
	if(lastfile && f!=lastfile)
		Bclean(lastfile->transcript);	/* save memory when multifile */
	lastfile = f;
	Fflush(f);
	if(f->marked)
		p0 = f->markp+sizeof(Mark)/RUNESIZE;
	else
		p0 = 0;
	f->dot = f->ndot;
	while((n=Bread(t, (Rune*)&buf, sizeof buf/RUNESIZE, p0)) > 0){
		switch(buf.cs.c){
		default:
			panic("unknown in Fupdate");
		case 'd':
			p1 = buf.cll.l;
			p2 = buf.cll.l1;
			p0 += sizeof(struct _cll)/RUNESIZE;
			if(p2 <= f->dot.r.p1)
				deltadot -= p2-p1;
			if(p2 <= f->mark.p1)
				deltamark -= p2-p1;
			p1 += delta, p2+=delta;
			delta -= p2-p1;
			if(!mktrans)
				for(p = p1; p<p2; p+=ni){
					if(p2-p>BLOCKSIZE)
						ni = BLOCKSIZE;
					else
						ni = p2-p;
					puthdr_csl(u, 'i', ni, p1);
					Bread(f->buf, tmp, ni, p);
					Binsert(u, ftempstr(tmp, ni), u->nrunes);
				}
			f->nrunes -= p2-p1;
			Bdelete(f->buf, p1, p2);
			changes = TRUE;
			break;

		case 'f':
			n = buf.cs.s;
			p0 += sizeof(struct _cs)/RUNESIZE;
			Strinsure(&genstr, n+1);
			Bread(t, tmp, n, p0);
			tmp[n] = 0;
			p0 += n;
			Strdupl(&genstr, tmp);
			if(!mktrans){
				puthdr_cs(u, 'f', f->name.n);
				Binsert(u, &f->name, u->nrunes);
			}
			Strduplstr(&f->name, &genstr);
			sortname(f);
			changes = TRUE;
			break;

		case 'i':
			n = buf.csl.s;
			p1 = buf.csl.l;
			p0 += sizeof(struct _csl)/RUNESIZE;
			if(p1 < f->dot.r.p1)
				deltadot += n;
			if(p1 < f->mark.p1)
				deltamark += n;
			p1 += delta;
			delta += n;
			if(!mktrans)
				puthdr_cll(u, 'd', p1, p1+n);
			changes = TRUE;
			f->nrunes += n;
			while(n > 0){
				if(n > BLOCKSIZE)
					ni = BLOCKSIZE;
				else
					ni = n;
				Bread(t, tmp, ni, p0);
				Binsert(f->buf, ftempstr(tmp, ni), p1);
				n -= ni;
				p1 += ni;
				p0 += ni;
			}
			break;
		}
	}
	toterminal(f, toterm);
	f->dot.r.p1 += deltadot;
	f->dot.r.p2 += deltadot;
	if(f->dot.r.p1 > f->nrunes)
		f->dot.r.p1 = f->nrunes;
	if(f->dot.r.p2 > f->nrunes)
		f->dot.r.p2 = f->nrunes;
	f->mark.p1 += deltamark;
	f->mark.p2 += deltamark;
	if(f->mark.p1 > f->nrunes)
		f->mark.p1 = f->nrunes;
	if(f->mark.p2 > f->nrunes)
		f->mark.p2 = f->nrunes;
	if(n < 0)
		panic("Fupdate read");
	if(f == cmd)
		f->mod = 0;	/* can't undo command file */
	if(p0 > f->markp+sizeof(Posn)/RUNESIZE){	/* for undo, this throws away the undo transcript */
		if(f->mod > 0){	/* can't undo the dawn of time */
			Bdelete(t, f->markp+sizeof(Mark)/RUNESIZE, t->nrunes);
			/* copy the undo list back into the transcript */
			for(p = 0; p<u->nrunes; p+=ni){
				if(u->nrunes-p>BLOCKSIZE)
					ni = BLOCKSIZE;
				else
					ni = u->nrunes-p;
				Bread(u, tmp, ni, p);
				Binsert(t, ftempstr(tmp, ni), t->nrunes);
			}
		}
		Bdelete(u, (Posn)0, u->nrunes);
	}
	return f==cmd? FALSE : changes;
}

void
puthdr_csl(Buffer *b, char c, short s, Posn p)
{
	struct _csl buf;

	if(p < 0)
		panic("puthdr_csP");
	buf.c = c;
	buf.s = s;
	buf.l = p;
	Binsert(b, ftempstr((Rune*)&buf, sizeof buf/RUNESIZE), b->nrunes);
}

void
puthdr_cs(Buffer *b, char c, short s)
{
	struct _cs buf;

	buf.c = c;
	buf.s = s;
	Binsert(b, ftempstr((Rune*)&buf, sizeof buf/RUNESIZE), b->nrunes);
}

void
puthdr_M(Buffer *b, Posn p, Range dot, Range mk, Mod m, short s1)
{
	Mark mark;
	static first = 1;

	if(!first && p<0)
		panic("puthdr_M");
	mark.p = p;
	mark.dot = dot;
	mark.mark = mk;
	mark.m = m;
	mark.s1 = s1;
	Binsert(b, ftempstr((Rune *)&mark, sizeof mark/RUNESIZE), b->nrunes);
}

void
puthdr_cll(Buffer *b, char c, Posn p1, Posn p2)
{
	struct _cll buf;

	if(p1<0 || p2<0)
		panic("puthdr_cll");
	buf.c = c;
	buf.l = p1;
	buf.l1 = p2;
	Binsert(b, ftempstr((Rune*)&buf, sizeof buf/RUNESIZE), b->nrunes);
}

long
Fchars(File *f, Rune *addr, Posn p1, Posn p2)
{
	return Bread(f->buf, addr, p2-p1, p1);
}

int
Fgetcset(File *f, Posn p)
{
	if(p<0 || p>f->nrunes)
		panic("Fgetcset out of range");
	if((f->ngetc = Fchars(f, f->getcbuf, p, p+NGETC))<0)
		panic("Fgetcset Bread fail");
	f->getcp = p;
	f->getci = 0;
	return f->ngetc;
}

int
Fbgetcset(File *f, Posn p)
{
	if(p<0 || p>f->nrunes)
		panic("Fbgetcset out of range");
	if((f->ngetc = Fchars(f, f->getcbuf, p<NGETC? (Posn)0 : p-NGETC, p))<0)
		panic("Fbgetcset Bread fail");
	f->getcp = p;
	f->getci = f->ngetc;
	return f->ngetc;
}

int
Fgetcload(File *f, Posn p)
{
	if(Fgetcset(f, p)){
		--f->ngetc;
		f->getcp++;
		return f->getcbuf[f->getci++];
	}
	return -1;
}

int
Fbgetcload(File *f, Posn p)
{
	if(Fbgetcset(f, p)){
		--f->getcp;
		return f->getcbuf[--f->getci];
	}
	return -1;
}

static String*
ftempstr(Rune *s, int n)
{
	static String p;

	p.s = s;
	p.n = n;
	p.size = n;
	return &p;
}
