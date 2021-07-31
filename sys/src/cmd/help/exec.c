#include <u.h>
#include <libc.h>
#include <libg.h>
#include <frame.h>
#include <regexp.h>
#include <bio.h>
#include "dat.h"
#include "fns.h"

int	maxargc;
int	argc;
char	**argv;
Rune	*qbuf;
ulong	maxqbuf;

void	Xcut(int, char*[], Page*, Text*);
void	Xexit(int, char*[], Page*, Text*);
void	Xopen(int, char*[], Page*, Text*);
void	Xnew(int, char*[], Page*, Text*);
void	Xpaste(int, char*[], Page*, Text*);
void	Xsnarf(int, char*[], Page*, Text*);
void	Xpattern(int, char*[], Page*, Text*);
void	Xtext(int, char*[], Page*, Text*);
void	Xsplit(int, char*[], Page*, Text*);
void	Xwrite(int, char*[], Page*, Text*);
void	Xput(int, char*[], Page*, Text*);
void	Xget(int, char*[], Page*, Text*);
void	Xclose(int, char*[], Page*, Text*);
void	Xgoto(int, char*[], Page*, Text*);

struct
{
	char	*name;
	void	(*f)(int, char*[], Page*, Text*);
}builtin[] = {
	"Cut",		Xcut,
	"Exit",		Xexit,
	"Xopen",	Xopen,	/* note change of name! */
	"New",		Xnew,
	"Paste",	Xpaste,
	"Pattern",	Xpattern,
	"Text",		Xtext,
	"Snarf",	Xsnarf,
	"Split",	Xsplit,
	"Write",	Xwrite,
	"Get!",		Xget,
	"Put!",		Xput,
	"Close!",	Xclose,
	"Open",		Xgoto,	/* note change of name! */
	0,		0,
};

/*
 * Put str, preceded by a tab, in tag; also remove it from tag.
 */

void
puttag(Page *p, char *str, ulong q0)
{
	Rune *r;

	textinsert(&p->tag, L"\t", q0, 0, 0);
	r = tmprstr(str);
	textinsert(&p->tag, r, q0+1, 0, 0);
	free(r);
}

void
unputtag(Page *p, char *str)
{
	int l;
	Rune *r, *s;
	Text *t;


	t = &p->tag;
	s = t->s;
	if(s == 0)
		return;
	r = tmprstr(str);
	l = rstrlen(r);
	while(s = rstrchr(s, '\t')){	/* assign = */
		s++;
		if(rstrncmp(s, r, l)==0 && strchr(" \t", s[l])){
			t->q0 = (s-t->s)-1;
			t->q1 = t->q0+l+1;
			cut(t, 0);
			break;
		}
	}
	free(r);
}

void
lookup(char *s)
{
	int i;
	char *n;

	for(i=0; n=builtin[i].name; i++){
		if(strcmp(s, n) == 0){
			puttag(rootpage, n, 0);
			bflush();
			(*builtin[i].f)(argc, argv, page, curt);
			unputtag(rootpage, n);
			return;
		}
	}
	xtrn(argc, argv, page, curt);
}

void
freeargs(void)
{
	int i;

	for(i=0; i<argc; i++)
		free(argv[i]);
	argc = 0;
}

void
newarg(Rune *str, ulong n)
{
	char *s;

	if(maxargc <= argc+1){	/* room for null pointer at argv[argc] */
		maxargc += 10;
		argv = erealloc(argv, maxargc*sizeof(char*));
	}
	s = cstr(str, n);
	argv[argc++] = s;
	argv[argc] = 0;
}

void
extend(Text *t, ulong *pp0, ulong *pp1, char *blanks)
{
	ulong p0, p1;

	p0 = *pp0;
	p1 = *pp1;
	while(p0>0 && utfrune(blanks, t->s[p0-1])==0)
		--p0;
	while(p1<t->n && utfrune(blanks, t->s[p1])==0)
		p1++;
	*pp0 = p0;
	*pp1 = p1;
}

ulong
nextarg(Text *t, ulong p0, ulong p1, char *blanks)
{
	ulong p;
	int i;

	while(p0<p1 && utfrune(blanks, t->s[p0]))
		p0++;
	if(p1>p0 && t->s[p0]=='\''){	/* quoted strings */
		p0++;	/* skip ' */
		i = 0;
		while(p0 < p1){
			if(t->s[p0] == '\''){
				p0++;
				if(p0==p1 || t->s[p0]!='\'')
					break;
			}
			if(i <= maxqbuf){
				maxqbuf += 64;
				qbuf = erealloc(qbuf, maxqbuf*sizeof(Rune));
			}
			qbuf[i++] = t->s[p0++];
		}
		newarg(qbuf, i);
		return p0;
	}
	p = p0;
	while(p0<p1 && utfrune(blanks, t->s[p0])==0)
		p0++;
	if(p < p1)
		newarg(t->s+p, p0-p);
	return p0;
}

void
args(Text *t, ulong p0, ulong p1)
{
	freeargs();

	/*
	 * Extract command and initial arguments
	 */
	if(p0 == p1){
		extend(t, &p0, &p1, " \t\n");
		if(p1 == p0)
			return;
		newarg(t->s+p0, p1-p0);
	}else{
		while(p1 > p0)
			p0 = nextarg(t, p0, p1, " \t\n");
	}
	/* BUG: should be able to pass selection */
}

void
execute(Text *t, ulong p0, ulong p1)
{
	args(t, p0, p1);
	if(argc)
		lookup(argv[0]);
}

void
Xcut(int argc, char *argv[], Page *page, Text *curt)
{
	USED(argc, argv, page);
	if(curt)
		cut(curt, 1);
}

void
Xpaste(int argc, char *argv[], Page *page, Text *curt)
{
	USED(argc, argv, page);
	if(curt)
		paste(curt);
}

void
Xsnarf(int argc, char *argv[], Page *page, Text *curt)
{
	USED(argc, argv, page);
	if(curt)
		snarf(curt);
}

void
Xsplit(int argc, char *argv[], Page *page, Text *curt)
{
	USED(argc, argv, curt);
	pagesplit(page, 1-page->parent->ver);
}

void
Xexit(int argc, char *argv[], Page *page, Text *curt)
{
	USED(argc, argv, page, curt);
	exits(0);
}

/*
 * Exact match
 */
Page*
findopen1(Page *p, Rune *name)
{
	Rune *s;
	int n;
	Page *q;

    Again:
	if(p == 0)
		return p;
	s = p->tag.s;
	if(s){
		n = rstrlen(name);
		if(p->tag.n>=n && rstrncmp(name, s, n)==0)
		if(p->tag.n==n || utfrune(" \t", s[n]))
			return p;
	}
	q = findopen1(p->down, name);
	if(q)
		return q;
	p = p->next;
	goto Again;
}

/*
 * Basename match; exact match is known not to work
 */
Page*
findopen2(Page *p, Rune *name)
{
	Rune *s, *t, *u;
	int n;
	Page *q;

    Again:
	if(p == 0)
		return p;
	s = p->tag.s;
	if(s && rstrchr(s, '/')){
		for(t=s; !utfrune(" \t", *t); t++)
			;
		u = s-1;
		do{
			s = u+1;
			u = rstrchr(s, '/');
		}while(u && u < t);
		n = rstrlen(name);
		if(p->tag.n>=n && rstrncmp(name, s, n)==0)
		if(p->tag.n==n || utfrune(" \t", s[n]))
			return p;
	}
	q = findopen2(p->next, name);
	if(q)
		return q;
    Down:
	p = p->down;
	goto Again;
}

Page*
findopen(Rune *name, int exact)
{
	Page *p;

	if(name[0] == 0)
		return 0;
	p = findopen1(rootpage, name);
	if(!exact && !p && !rstrchr(name, '/'))
		p = findopen2(rootpage, name);
	if(p){
		/* make sure we can see it */
		if(!p->visible || p->body.maxlines<2)
			pagetop(p, p->r.min, 0);
	}
	return p;
}

Rune*
endslash(Rune *an)
{
	Rune *n;

	n = an;
	if(n==0 || *n==0)
		return 0;
	while(*n){
		if(utfrune(" \t", *n))
			break;
		n++;
	}
	while(--n >= an)
		if(*n == '/')
			return n+1;
	return 0;
}

void
namefix(char *n)
{
	char *e, *p, *q, *r;

	e = n+strlen(n);
	/* reduce multiple slashes */
	for(q=n; q<e; q++)
		while(q[0]=='/' && q[1]=='/'){
			memmove(q+1, q+2, e-(q+2)+1);
			--e;
		}
	/* lose ./ */
	for(q=n; q<e; q++)
		while(q[0]=='/' && q[1]=='.' && (q[2]=='/' || q+2==e)){
			memmove(q+1, q+3, e-(q+3)+1);
			--e;
		}
    Again:
	/* reduce initial /../ */
	p = n;
	while(p[0]=='/' && p[1]=='.' && p[2]=='.' && p[3]=='/'){
		memmove(p, p+3, e-(p+3)+1);
		e -= 3;
	}
	/* skip initial ../ */
	p = n;
	while(p[0]=='.' && p[1]=='.' && p[2]=='/')
		p += 3;
	/* look for /..; can't be at beginning */
	for(q=p; q<e; q++)
		if(q[0]=='/' && q[1]=='.' && q[2]=='.' && (q[3]=='/' || q+3==e))
			break;
	if(q < e){
		/* q points to /.. */
		r = q;
		q += 3;
		while(r>p && r[-1]!='/')
			--r;
		if(r > p)
			--r;
		/* r points to /file/.. */
		memmove(r, q, e-q+1);
		e -= q-r;
		goto Again;
	}
}

Page*
open1(int argc, char *argv[], Page *page, Text *curt, Page* (*f)(char*, Page*))
{
	int n;
	Rune *name, *name1;
	Rune *d0, *d1;
	ulong p0, p1;
	char *c;
	Page *p, *parent;

	p = 0;
	if(argc > 1){
		if(curt)
			parent = curt->page->parent;
		else
			parent = page->parent;
		for(n=1; n<argc; n++)
			p = (*f)(argv[n], parent);
	}else{
		if(curt==0 || curt->n==0)
			return 0;
		p0 = curt->q0;
		p1 = curt->q1;
		d1 = 0;
		if(p0 == p1){
			d1 = endslash(curt->page->tag.s);
			extend(curt, &p0, &p1, " \t\n\"'<>");
		}
		n = p1-p0;
		if(n == 0)
			return 0;
		/*
		 * If can derive a directory name and file name
		 * does not begin with / prepend directory name
		 */
		if(d1 && curt->s[p0]!='/'){
			d0 = curt->page->tag.s;
			name = emalloc(((d1-d0)+n+1)*sizeof(Rune));
			memmove(name, d0, (d1-d0)*sizeof(Rune));
			memmove(name+(d1-d0), curt->s+p0, n*sizeof(Rune));
			name[(d1-d0)+n] = 0;
		}else{
			name = emalloc((n+1)*sizeof(Rune));
			memmove(name, curt->s+p0, n*sizeof(Rune));
			name[n] = 0;
		}
		/* toss leading ./  */
		name1 = name;
		if(rstrncmp(name, L"./", 2)==0){
			name += 2;
			while(*name == '/')
				name++;
			if(*name == 0)	/* whoops */
				name = name1;
		}
		c = cstr(name, rstrlen(name));
		p = (*f)(c, curt->page->parent);
		free(c);
		free(name1);
	}
	if(p)
		newsel(&p->body);
	return p;
}

void
readfile(int fd, Text *t, ulong q0)
{
	Dir d;
	int i, c, ne, ndir;
	Rune *r;
	uchar *s, *dir;
	ulong q1;
	Biobuf *b;
	uchar buf[DIRLEN];

	dirfstat(fd, &d);
	q1 = q0;
	if(d.mode & CHDIR){
		ndir = 0;
		ne = 0;
		dir = 0;
		while(read(fd, buf, DIRLEN) > 0){
			if(ndir == ne){
				ndir += 32;
				dir = erealloc(dir, ndir*(NAMELEN+1));
			}
			c = strlen((char*)buf)+2;
			s = dir+ne*(NAMELEN+1);
			memmove(s, buf, c);
			s[c-2] = '\n';
			s[c-1] = 0;
			ne++;
		}
		if(dir){
			qsort(dir, ne, NAMELEN+1, (int(*)(void*,void*))strcmp);
			for(i=0; i<ne; i++){
				s = dir+i*(NAMELEN+1);
				r = tmprstr((char*)s);
				c = rstrlen(r);
				Strinsert(t, r, c, q1);
				free(r);
				q1 += c;
			}
			free(dir);
		}	
	}else{
		b = emalloc(sizeof(Biobuf));
		Binit(b, fd, OREAD);
		i = 0;
		while((c = Bgetrune(b)) >= 0){
			genbuf[i++] = c;
			if(i >= GENBUFSIZE-1){
				Strinsert(t, genbuf, i, q1);
				q1 += i;
				i = 0;
			}
		}
		if(i){
			Strinsert(t, genbuf, i, q1);
			q1 += i;
		}
		Bclose(b);
		free(b);
	}
	if(t->s){
		if(q0 >= t->org)
			frinsert(t, t->s+q0, t->s+q1, q0-t->org);
		else if(q0 < t->org)
			t->org += q1-q0;
	}
	t->q0 = q0;
	t->q1 = q1;
}

Page*
doopen(char *name, Page *parent)
{
	int fd, n;
	Page *p;
	Dir d;
	char *dir;
	Rune *rname;

	namefix(name);
	rname = tmprstr(name);
	p = findopen(rname, 0);
	if(p){
		free(rname);
		return p;
	}
	fd = open(name, OREAD);
	if(fd<0 || dirfstat(fd, &d)<0){
		close(fd);
		free(rname);
		err("%s: %r\n", name);
		return 0;
	}
	dir = 0;
	if(d.mode & CHDIR){
		n = rstrlen(rname);
		if(rname[n-1] != '/'){
			n = strlen(name);
			dir = emalloc(n+2);
			memmove(dir, name, n);
			dir[n] = '/';
			dir[n+1] = 0;
			free(rname);
			rname = tmprstr(dir);
			p = findopen(rname, 0);
			if(p){
				free(rname);
				close(fd);
				return p;
			}
		}
	}
	p = pageadd(parent, 1);
	if(p == 0){
		free(rname);
		return 0;
	}
	textinsert(&p->tag, rname, 0, 0, 1);
	textinsert(&p->tag, L"\tClose!\tGet!\t", p->tag.n, 0, 1);

	readfile(fd, &p->body, 0);
	close(fd);
	p->body.q0 = 0;
	p->body.q1 = 0;
	highlight(&p->body);
	newsel(&p->body);
	if(dir)
		free(dir);
	free(rname);
	return p;
}

ulong
rstrtoul(Rune *r, Rune **rp, int base)
{
	char buf[64], *q;
	ulong l;
	int i;

	for(i=0; i<sizeof buf-1 && r[i]; i++)
		buf[i] = r[i];	/* no need to use runetochar */
	buf[i] = 0;
	q = 0;
	l = strtoul(buf, &q, base);
	if(q)
		*rp = r+(q-buf);
	return l;
}

Addr
address(Rune *c, String *t)
{
	Addr a;
	int l0, l1;
	ulong i, p, q, e;
	Rune *s;

	a.q0 = -1;
	a.q1 = -1;
	l0 = -1;
	l1 = -1;
	a.valid = 1;
	if(c[0] == '#'){
		c++;
		a.q0 = rstrtoul(c, &c, 10);
	}else{
		l0 = rstrtoul(c, &c, 10);
	}
	if(c[0]==','){
		c = c+1;
		if(c[0] == '#'){
			c++;
			a.q1 = rstrtoul(c, 0, 10);
		}else
			l1 = rstrtoul(c, 0, 10);
	}
	if(l0>=0 || l1>=0){	/* convert lines into chars */
		s = t->s;
		if(l0>=0 && l1>=0 && l1<l0){
			l1 = l0;
			a.valid = 0;
		}
		if(l1 >= 0)
			e = l1;
		else
			e = l0;
		p = 0;
		q = 0;
		e++;	/* goto end of line */
		for(i=1; i<e; i++){
			if(i<=l0)
				q = p;
			while(p < t->n){
				p++;
				if(*s++ == '\n')
					break;
			}
		}
		if(a.q0 < 0)
			a.q0 = q;
		if(a.q1 < 0)
			a.q1 = p;
	}
	if(a.q0 > t->n)
		a.q0 = t->n;
	if(a.q1 > t->n)
		a.q1 = t->n;
	if(a.q1 < a.q0){
		a.q1 = a.q0;
		a.valid = 0;
	}
	return a;
}

Page*
dogoto(char *name, Page *parent)
{
	Addr a;
	Rune *r;
	char *l;
	Page *w;

	l = utfrune(name, ':');
	if(l)
		*l = 0;
	w = doopen(name, parent);
	if(w == 0)
		return 0;
	newsel(&w->body);
	if(l){
		r = tmprstr(l+1);
		a = address(r, &w->body);
		free(r);
		if(a.valid){
			w->body.q0 = a.q0;
			w->body.q1 = a.q1;
			show(&w->body, w->body.q0);
			highlight(&w->body);
		}
	}
	return w;
}

Page*
donew(char *name, Page *parent)
{
	int fd, n;
	Page *p;
	Dir d;
	char *dir;
	Rune *rname;

	namefix(name);
	rname = tmprstr(name);
	p = findopen(rname, 0);
	if(p){
		free(rname);
		return p;
	}
	fd = open(name, OREAD);
	dir = 0;
	if(fd>=0 && dirfstat(fd, &d)>=0 && (d.mode&CHDIR)){
		n = strlen(name);
		if(name[n-1] != '/'){
			dir = emalloc(n+2);
			memmove(dir, name, n);
			dir[n] = '/';
			dir[n+1] = 0;
			name = dir;
			free(rname);
			rname = tmprstr(name);
			p = findopen(rname, 0);
			if(p)
				goto Return;
		}
	}
	p = pageadd(parent, 1);
	if(p == 0)
		goto Return;
	textinsert(&p->tag, rname, 0, 0, 1);
	textinsert(&p->tag, L"\tClose!\tGet!\t", p->tag.n, 0, 1);
	if(fd >= 0){
		readfile(fd, &p->body, 0);
		close(fd);
	}
	p->body.q0 = 0;
	p->body.q1 = 0;
	highlight(&p->body);
	newsel(&p->body);
	if(dir)
		free(dir);
    Return:
	free(rname);
	close(fd);
	return p;
}


void
Xopen(int argc, char *argv[], Page *page, Text *curt)
{
	open1(argc, argv, page, curt, doopen);
}

void
Xgoto(int argc, char *argv[], Page *page, Text *curt)
{
	open1(argc, argv, page, curt, dogoto);
}

void
Xnew(int argc, char *argv[], Page *page, Text *curt)
{
	Page *p;

	if(open1(argc, argv, page, curt, donew) == 0){
		p = pageadd(page->parent, 1);
		textinsert(&p->tag, L"\tClose!\tGet!\t", 0, 0, 1);
	}
}

void
Xwrite(int argc, char *argv[], Page *page, Text *curt)
{
	int n;
	long i;
	char *name;
	Rune *s;
	Text *tag, *body;
	Biobuf *b;

	USED(argc, argv, page);
	if(curt == 0)
		return;
	tag = &curt->page->tag;
	body = &curt->page->body;
	s = curt->s+curt->q0;
	n = curt->q1-curt->q0;
	if(n == 0){
		s = tag->s;
		for(n=0; s[n] && s[n]!=' ' && s[n]!='\t'; n++)
			;
	}
	name = cstr(s, n);
	b = Bopen(name, OWRITE);
	if(b == 0){
		err("%s: %r\n", name);
		free(name);
		return;
	}
	free(name);
	for(i=0; i<body->n; i++)
		if(Bputrune(b, body->s[i]) == Beof){
			err("write error: %r\n");
			break;
		}
	Bclose(b);
}

void
Xput(int argc, char *argv[], Page *page, Text *curt)
{
	int n;
	long i;
	char *name;
	Rune *s;
	Text *tag, *body;
	Biobuf *b;

	USED(argc, argv, curt);
	tag = &page->tag;
	body = &page->body;
	s = tag->s;
	for(n=0; s[n] && s[n]!=' ' && s[n]!='\t'; n++)
		;
	name = cstr(s, n);
	b = Bopen(name, OWRITE);
	if(b == 0){
		err("%s: %f\n", name);
		free(name);
		return;
	}
	free(name);
	for(i=0; i<body->n; i++)
		if(Bputrune(b, body->s[i]) == Beof){
			err("write error: %r\n");
			Bclose(b);
			return;
		}
	unputtag(page, "Put!");
	page->mod = 0;
	Bclose(b);
}

void
Xget(int argc, char *argv[], Page *page, Text *curt)
{
	int fd, n;
	char *name;
	Rune *s;
	Text *body;

	USED(argc, argv, curt);
	body = &page->body;
	s = page->tag.s;
	for(n=0; s[n] && s[n]!=' ' && s[n]!='\t'; n++)
		;
	name = cstr(s, n);
	fd = open(name, OREAD);
	free(name);
	if(fd < 0)
		return;

	frdelete(body, 0, body->nchars);
	Strdelete(body, 0, body->n);
	body->q0 = 0;
	body->q1 = 0;
	readfile(fd, body, 0);
	close(fd);
	body->q0 = 0;
	body->q1 = 0;
	show(body, 0);
	highlight(body);
	newsel(body);
	unputtag(page, "Put!");
	page->mod = 0;
}

void
Xclose(int argc, char *argv[], Page *page, Text *curt)
{
	USED(argc, argv, curt);
	if(page==page->parent->down && page->next==0)
		return;		/* can't delete last page in row */
	pagetop(page, page->r.min, 1);
}

jmp_buf	regjmp;
Reprog	*reprog;

void
regerror(char *s)
{
	err("re error: %s\n", s);
	longjmp(regjmp, 1);
}

void
search(int argc, char *argv[], Text *curt, Reprog *(*rc)(char*))
{
	char *pat;
	ulong n;
	Resub match[10];
	int again;
	ulong p1;

	if(setjmp(regjmp))
		return;
	if(curt == 0)
		return;
	n = curt->q1-curt->q0;
	if(argc > 1){
		pat = argv[1];
		reprog = (*rc)(pat);
		if(reprog == 0)
			return;
	}else if(n > 0){
		free(reprog);
		reprog = 0;
		pat = cstr(curt->s+curt->q0, n);
		reprog = (*rc)(pat);
		free(pat);
		if(reprog == 0)
			return;
	}else if(reprog == 0){
		err("no pattern\n");
		return;
	}
	again = 0;
	p1 = curt->q1;
   scan:
	match[0].rsp = curt->s+p1;
	match[0].rep = curt->s+curt->n;
	if(rregexec(reprog, curt->s, match, sizeof match/sizeof(Resub)))
		goto Found;
	match[0].rsp = curt->s;
	match[0].rep = curt->s+curt->n;
	if(rregexec(reprog, curt->s, match, sizeof match/sizeof(Resub)))
		goto Found;

	err("no match\n");
	return;

    Found:
	/*
	 * A null match appended to the current string
	 * doesn't count; advance and try again
	 */
	if(!again && match[0].rsp==match[0].rep && curt->q1==match[0].rsp-curt->s){
		p1++;
		if(p1 > curt->n)
			p1 = 0;
		again = 1;
		goto scan;
	}
	curt->q0 = match[0].rsp-curt->s;
	curt->q1 = match[0].rep-curt->s;
	show(curt, curt->q0);
	highlight(curt);
	newsel(curt);
}

void
Xpattern(int argc, char *argv[], Page *page, Text *curt)
{
	USED(page);
	search(argc, argv, curt, regcomp);
}

void
Xtext(int argc, char *argv[], Page *page, Text *curt)
{
	USED(page);
	search(argc, argv, curt, regcomplit);
}
