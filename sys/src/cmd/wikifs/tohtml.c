#include <u.h>
#include <libc.h>
#include <bio.h>
#include <String.h>
#include <thread.h>
#include "wiki.h"

/*
 * Get HTML and text templates from underlying file system.
 * Caches them, which means changes don't take effect for
 * up to Tcache seconds after they are made.
 * 
 * If the files are deleted, we keep returning the last
 * known copy.
 */
enum {
	WAIT = 60
};

static char *name[2*Ntemplate] = {
 [Tpage]		"page.html",
 [Tedit]		"edit.html",
 [Tdiff]		"diff.html",
 [Thistory]		"history.html",
 [Tnew]		"new.html",
 [Toldpage]	"oldpage.html",
 [Twerror]		"werror.html",
 [Ntemplate+Tpage]	"page.txt",
 [Ntemplate+Tdiff]	"diff.txt",
 [Ntemplate+Thistory]	"history.txt",
 [Ntemplate+Toldpage]	"oldpage.txt",
 [Ntemplate+Twerror]	"werror.txt",
};

static struct {
	RWLock;
	String *s;
	ulong t;
	Qid qid;
} cache[2*Ntemplate];

static void
cacheinit(void)
{
	int i;
	static int x;
	static Lock l;

	if(x)
		return;
	lock(&l);
	if(x){
		unlock(&l);
		return;
	}

	for(i=0; i<2*Ntemplate; i++)
		if(name[i])
			cache[i].s = s_copy("");
	x = 1;
	unlock(&l);
}

static String*
gettemplate(int type)
{
	int n;
	Biobuf *b;
	Dir *d;
	String *s, *ns;

	if(name[type]==nil)
		return nil;

	cacheinit();

	rlock(&cache[type]);
	if(0 && cache[type].t+Tcache >= time(0)){
		s = s_incref(cache[type].s);
		runlock(&cache[type]);
		return s;
	}
	runlock(&cache[type]);

//	d = nil;
	wlock(&cache[type]);
	if(0 && cache[type].t+Tcache >= time(0) || (d = wdirstat(name[type])) == nil)
		goto Return;

	if(0 && d->qid.vers == cache[type].qid.vers && d->qid.path == cache[type].qid.path){
		cache[type].t = time(0);
		goto Return;
	}

	if((b = wBopen(name[type], OREAD)) == nil)
		goto Return;

	ns = s_reset(nil);
	do
		n = s_read(b, ns, Bsize);
	while(n > 0);
	Bterm(b);
	if(n < 0) {
		s_free(ns);
		goto Return;
	}

	s_free(cache[type].s);
	cache[type].s = ns;
	cache[type].qid = d->qid;
	cache[type].t = time(0);

Return:
	free(d);
	s = s_incref(cache[type].s);
	wunlock(&cache[type]);
	return s;
}

	
/*
 * Write wiki document in HTML.
 */
static String*
s_escappend(String *s, char *p, int pre)
{
	char *q;

	while(q = strpbrk(p, pre ? "<>&" : " <>&")){
		s = s_nappend(s, p, q-p);
		switch(*q){
		case '<':
			s = s_append(s, "&lt;");
			break;
		case '>':
			s = s_append(s, "&gt;");
			break;
		case '&':
			s = s_append(s, "&amp;");
			break;
		case ' ':
			s = s_append(s, "\n");
		}
		p = q+1;
	}
	s = s_append(s, p);
	return s;
}

static char*
mkurl(char *s, int ty)
{
	char *p, *q;

	if(strncmp(s, "http:", 5)==0
	|| strncmp(s, "https:", 6)==0
	|| strncmp(s, "#", 1)==0
	|| strncmp(s, "ftp:", 4)==0
	|| strncmp(s, "mailto:", 7)==0
	|| strncmp(s, "telnet:", 7)==0
	|| strncmp(s, "file:", 5)==0)
		return estrdup(s);

	if(strchr(s, ' ')==nil && strchr(s, '@')!=nil){
		p = emalloc(strlen(s)+8);
		strcpy(p, "mailto:");
		strcat(p, s);
		return p;
	}

	if(ty == Toldpage)
		p = smprint("../../%s", s);
	else
		p = smprint("../%s", s);

	for(q=p; *q; q++)
		if(*q==' ')
			*q = '_';
	return p;
}

int okayinlist[Nwtxt] =
{
	[Wbullet]	1,
	[Wlink]	1,
	[Wman]	1,
	[Wplain]	1,
};

int okayinpre[Nwtxt] =
{
	[Wlink]	1,
	[Wman]	1,
	[Wpre]	1,
};

int okayinpara[Nwtxt] =
{
	[Wpara]	1,
	[Wlink]	1,
	[Wman]	1,
	[Wplain]	1,
};

char*
nospaces(char *s)
{
	char *q;
	s = strdup(s);
	if(s == nil)
		return nil;
	for(q=s; *q; q++)
		if(*q == ' ')
			*q = '_';
	return s;
}
	
String*
pagehtml(String *s, Wpage *wtxt, int ty)
{
	char *p, tmp[40];
	int inlist, inpara, inpre, t, tnext;
	Wpage *w;

	inlist = 0;
	inpre = 0;
	inpara = 0;

	for(w=wtxt; w; w=w->next){
		t = w->type;
		tnext = Whr;
		if(w->next)
			tnext = w->next->type;

		if(inlist && !okayinlist[t]){
			inlist = 0;
			s = s_append(s, "\n</li>\n</ul>\n");
		}
		if(inpre && !okayinpre[t]){
			inpre = 0;
			s = s_append(s, "</pre>\n");
		}

		switch(t){
		case Wheading:
			p = nospaces(w->text);
			s = s_appendlist(s, 
				"\n<a name=\"", p, "\" /><h3>", 
				w->text, "</h3>\n", nil);
			free(p);
			break;

		case Wpara:
			if(inpara){
				s = s_append(s, "\n</p>\n");
				inpara = 0;
			}
			if(okayinpara[tnext]){
				s = s_append(s, "\n<p class='para'>\n");
				inpara = 1;
			}
			break;

		case Wbullet:
			if(!inlist){
				inlist = 1;
				s = s_append(s, "\n<ul>\n");
			}else
				s = s_append(s, "\n</li>\n");
			s = s_append(s, "\n<li>\n");
			break;

		case Wlink:
			if(w->url == nil)
				p = mkurl(w->text, ty);
			else
				p = w->url;
			s = s_appendlist(s, "<a href=\"", p, "\">", nil);
			s = s_escappend(s, w->text, 0);
			s = s_append(s, "</a>");
			if(w->url == nil)
				free(p);
			break;

		case Wman:
			sprint(tmp, "%d", w->section);
			s = s_appendlist(s, 
				"<a href=\"http://plan9.bell-labs.com/magic/man2html/",
				tmp, "/", w->text, "\"><i>", w->text, "</i>(",
				tmp, ")</a>", nil);
			break;
			
		case Wpre:
			if(!inpre){
				inpre = 1;
				s = s_append(s, "\n<pre>\n");
			}
			s = s_escappend(s, w->text, 1);
			s = s_append(s, "\n");
			break;
		
		case Whr:
			s = s_append(s, "<hr />");
			break;

		case Wplain:
			s = s_escappend(s, w->text, 0);
			break;
		}
	}
	if(inlist)
		s = s_append(s, "\n</li>\n</ul>\n");
	if(inpre)
		s = s_append(s, "</pre>\n");
	if(inpara)
		s = s_append(s, "\n</p>\n");
	return s;
}

static String*
copythru(String *s, char **newp, int *nlinep, int l)
{
	char *oq, *q, *r;
	int ol;

	q = *newp;
	oq = q;
	ol = *nlinep;
	while(ol < l){
		if(r = strchr(q, '\n'))
			q = r+1;
		else{
			q += strlen(q);
			break;
		}
		ol++;
	}
	if(*nlinep < l)
		*nlinep = l;
	*newp = q;
	return s_nappend(s, oq, q-oq);
}

static int
dodiff(char *f1, char *f2)
{
	int p[2];

	if(pipe(p) < 0){
		return -1;
	}

	switch(fork()){
	case -1:
		return -1;

	case 0:
		close(p[0]);
		dup(p[1], 1);
		execl("/bin/diff", "diff", f1, f2, nil);
		_exits(nil);
	}
	close(p[1]);
	return p[0];
}


/* print document i grayed out, with only diffs relative to j in black */
static String*
s_diff(String *s, Whist *h, int i, int j)
{
	char *p, *q, *pnew;
	int fdiff, fd1, fd2, n1, n2;
	Biobuf b;
	char fn1[40], fn2[40];
	String *new, *old;
	int nline;

	if(j < 0)
		return pagehtml(s, h->doc[i].wtxt, Tpage);

	strcpy(fn1, "/tmp/wiki.XXXXXX");
	strcpy(fn2, "/tmp/wiki.XXXXXX");
	if((fd1 = opentemp(fn1)) < 0 || (fd2 = opentemp(fn2)) < 0){
		close(fd1);
		s = s_append(s, "\nopentemp failed; sorry\n");
		return s;
	}

	new = pagehtml(s_reset(nil), h->doc[i].wtxt, Tpage);
	old = pagehtml(s_reset(nil), h->doc[j].wtxt, Tpage);
	write(fd1, s_to_c(new), s_len(new));
	write(fd2, s_to_c(old), s_len(old));

	fdiff = dodiff(fn2, fn1);
	if(fdiff < 0)
		s = s_append(s, "\ndiff failed; sorry\n");
	else{
		nline = 0;
		pnew = s_to_c(new);
		Binit(&b, fdiff, OREAD);
		while(p = Brdline(&b, '\n')){
			if(p[0]=='<' || p[0]=='>' || p[0]=='-')
				continue;
			p[Blinelen(&b)-1] = '\0';
			if((p = strpbrk(p, "acd")) == nil)
				continue;
			n1 = atoi(p+1);
			if(q = strchr(p, ','))
				n2 = atoi(q+1);
			else
				n2 = n1;
			switch(*p){
			case 'a':
			case 'c':
				s = s_append(s, "<span class='old_text'>");
				s = copythru(s, &pnew, &nline, n1-1);
				s = s_append(s, "</span><span class='new_text'>");
				s = copythru(s, &pnew, &nline, n2);
				s = s_append(s, "</span>");
				break;
			}
		}
		close(fdiff);
		s = s_append(s, "<span class='old_text'>");
		s = s_append(s, pnew);
		s = s_append(s, "</span>");

	}
	s_free(new);
	s_free(old);
	close(fd1);
	close(fd2);
	return s;
}

static String*
diffhtml(String *s, Whist *h)
{
	int i;
	char tmp[50];
	char *atime;

	for(i=h->ndoc-1; i>=0; i--){
		s = s_append(s, "<hr /><div class='diff_head'>\n");
		if(i==h->current)
			sprint(tmp, "index.html");
		else
			sprint(tmp, "%lud", h->doc[i].time);
		atime = ctime(h->doc[i].time);
		atime[strlen(atime)-1] = '\0';
		s = s_appendlist(s, 
			"<a href=\"", tmp, "\">",
			atime, "</a>", nil);
		if(h->doc[i].author)
			s = s_appendlist(s, ", ", h->doc[i].author, nil);
		if(h->doc[i].conflict)
			s = s_append(s, ", conflicting write");
		s = s_append(s, "\n");
		if(h->doc[i].comment)
			s = s_appendlist(s, "<br /><i>", h->doc[i].comment, "</i>\n", nil);
		s = s_append(s, "</div><hr />");
		s = s_diff(s, h, i, i-1);
	}
	s = s_append(s, "<hr>");
	return s;
}

static String*
historyhtml(String *s, Whist *h)
{
	int i;
	char tmp[40];
	char *atime;

	s = s_append(s, "<ul>\n");
	for(i=h->ndoc-1; i>=0; i--){
		if(i==h->current)
			sprint(tmp, "index.html");
		else
			sprint(tmp, "%lud", h->doc[i].time);
		atime = ctime(h->doc[i].time);
		atime[strlen(atime)-1] = '\0';
		s = s_appendlist(s, 
			"<li><a href=\"", tmp, "\">",
			atime, "</a>", nil);
		if(h->doc[i].author)
			s = s_appendlist(s, ", ", h->doc[i].author, nil);
		if(h->doc[i].conflict)
			s = s_append(s, ", conflicting write");
		s = s_append(s, "\n");
		if(h->doc[i].comment)
			s = s_appendlist(s, "<br><i>", h->doc[i].comment, "</i>\n", nil);
	}
	s = s_append(s, "</ul>");
	return s;		
}

String*
tohtml(Whist *h, Wdoc *d, int ty)
{
	char *atime;
	char *p, *q, ver[40];
	int nsub;
	Sub sub[3];
	String *s, *t;

	t = gettemplate(ty);
	if(p = strstr(s_to_c(t), "PAGE"))
		q = p+4;
	else{
		p = s_to_c(t)+s_len(t);
		q = nil;
	}

	nsub = 0;
	if(h){
		sub[nsub] = (Sub){ "TITLE", h->title };
		nsub++;
	}
	if(d){
		sprint(ver, "%lud", d->time);
		sub[nsub] = (Sub){ "VERSION", ver };
		nsub++;
		atime = ctime(d->time);
		atime[strlen(atime)-1] = '\0';
		sub[nsub] = (Sub){ "DATE", atime };
		nsub++;
	}

	s = s_reset(nil);
	s = s_appendsub(s, s_to_c(t), p-s_to_c(t), sub, nsub);
	switch(ty){
	case Tpage:
	case Toldpage:
		s = pagehtml(s, d->wtxt, ty);
		break;
	case Tedit:
		s = pagetext(s, d->wtxt, 0);
		break;
	case Tdiff:
		s = diffhtml(s, h);
		break;
	case Thistory:
		s = historyhtml(s, h);
		break;
	case Tnew:
	case Twerror:
		break;
	}
	if(q)
		s = s_appendsub(s, q, strlen(q), sub, nsub);
	s_free(t);
	return s;
}

enum {
	LINELEN = 70,
};

static String*
s_appendbrk(String *s, char *p, char *prefix, int dosharp)
{
	char *e, *w, *x;
	int first, l;
	Rune r;

	first = 1;
	while(*p){
		s = s_append(s, p);
		e = strrchr(s_to_c(s), '\n');
		if(e == nil)
			e = s_to_c(s);
		else
			e++;
		if(utflen(e) <= LINELEN)
			break;
		x = e; l=LINELEN;
		while(l--)
			x+=chartorune(&r, x);
		x = strchr(x, ' ');
		if(x){
			*x = '\0';
			w = strrchr(e, ' ');
			*x = ' ';
		}else
			w = strrchr(e, ' ');
	
		if(w-s_to_c(s) < strlen(prefix))
			break;
		
		x = estrdup(w+1);
		*w = '\0';
		s->ptr = w;
		s_append(s, "\n");
		if(dosharp)
			s_append(s, "#");
		s_append(s, prefix);
		if(!first)
			free(p);
		first = 0;
		p = x;
	}
	if(!first)
		free(p);
	return s;
}

static void
s_endline(String *s, int dosharp)
{
	if(dosharp){
		if(s->ptr == s->base+1 && s->ptr[-1] == '#')
			return;

		if(s->ptr > s->base+1 && s->ptr[-1] == '#' && s->ptr[-2] == '\n')
			return;
		s_append(s, "\n#");
	}else{
		if(s->ptr > s->base+1 && s->ptr[-1] == '\n')
			return;
		s_append(s, "\n");
	}
}

String*
pagetext(String *s, Wpage *page, int dosharp)
{
	int inlist, inpara;
	char *prefix, *sharp, tmp[40];
	String *t;
	Wpage *w;

	inlist = 0;
	inpara = 0;
	prefix = "";
	sharp = dosharp ? "#" : "";
	s = s_append(s, sharp);
	for(w=page; w; w=w->next){
		switch(w->type){
		case Wheading:
			if(inlist){
				prefix = "";
				inlist = 0;
			}
			s_endline(s, dosharp);
			if(!inpara){
				inpara = 1;
				s = s_appendlist(s, "\n", sharp, nil);
			}
			s = s_appendlist(s, w->text, "\n", sharp, "\n", sharp, nil);
			break;

		case Wpara:
			s_endline(s, dosharp);
			if(inlist){
				prefix = "";
				inlist = 0;
			}
			if(!inpara){
				inpara = 1;
				s = s_appendlist(s, "\n", sharp, nil);
			}
			break;

		case Wbullet:
			s_endline(s, dosharp);
			if(!inlist)
				inlist = 1;
			if(inpara)
				inpara = 0;
			s = s_append(s, " *\t");
			prefix = "\t";
			break;

		case Wlink:
			if(inpara)
				inpara = 0;
			t = s_append(s_copy("["), w->text);
			if(w->url == nil)
				t = s_append(t, "]");
			else{
				t = s_append(t, " | ");
				t = s_append(t, w->url);
				t = s_append(t, "]");
			}
			s = s_appendbrk(s, s_to_c(t), prefix, dosharp);
			s_free(t);
			break;

		case Wman:
			if(inpara)
				inpara = 0;
			s = s_appendbrk(s, w->text, prefix, dosharp);
			sprint(tmp, "(%d)", w->section);
			s = s_appendbrk(s, tmp, prefix, dosharp);
			break;
			
		case Wpre:
			if(inlist){
				prefix = "";
				inlist = 0;
			}
			if(inpara)
				inpara = 0;
			s_endline(s, dosharp);
			s = s_appendlist(s, "! ", w->text, "\n", sharp, nil);
			break;
		case Whr:
			s_endline(s, dosharp);
			s = s_appendlist(s, "------------------------------------------------------ \n", sharp, nil);
			break;

		case Wplain:
			if(inpara)
				inpara = 0;
			s = s_appendbrk(s, w->text, prefix, dosharp);
			break;
		}
	}
	s_endline(s, dosharp);
	s->ptr--;
	*s->ptr = '\0';
	return s;
}

static String*
historytext(String *s, Whist *h)
{
	int i;
	char tmp[40];
	char *atime;

	for(i=h->ndoc-1; i>=0; i--){
		if(i==h->current)
			sprint(tmp, "[current]");
		else
			sprint(tmp, "[%lud/]", h->doc[i].time);
		atime = ctime(h->doc[i].time);
		atime[strlen(atime)-1] = '\0';
		s = s_appendlist(s, " * ", tmp, " ", atime, nil);
		if(h->doc[i].author)
			s = s_appendlist(s, ", ", h->doc[i].author, nil);
		if(h->doc[i].conflict)
			s = s_append(s, ", conflicting write");
		s = s_append(s, "\n");
		if(h->doc[i].comment)
			s = s_appendlist(s, "<i>", h->doc[i].comment, "</i>\n", nil);
	}
	return s;		
}

String*
totext(Whist *h, Wdoc *d, int ty)
{
	char *atime;
	char *p, *q, ver[40];
	int nsub;
	Sub sub[3];
	String *s, *t;

	t = gettemplate(Ntemplate+ty);
	if(p = strstr(s_to_c(t), "PAGE"))
		q = p+4;
	else{
		p = s_to_c(t)+s_len(t);
		q = nil;
	}

	nsub = 0;
	if(h){
		sub[nsub] = (Sub){ "TITLE", h->title };
		nsub++;
	}
	if(d){
		sprint(ver, "%lud", d->time);
		sub[nsub] = (Sub){ "VERSION", ver };
		nsub++;
		atime = ctime(d->time);
		atime[strlen(atime)-1] = '\0';
		sub[nsub] = (Sub){ "DATE", atime };
		nsub++;
	}
	
	s = s_reset(nil);
	s = s_appendsub(s, s_to_c(t), p-s_to_c(t), sub, nsub);
	switch(ty){
	case Tpage:
	case Toldpage:
		s = pagetext(s, d->wtxt, 0);
		break;
	case Thistory:
		s = historytext(s, h);
		break;
	case Tnew:
	case Twerror:
		break;
	}
	if(q)
		s = s_appendsub(s, q, strlen(q), sub, nsub);
	s_free(t);
	return s;
}

String*
doctext(String *s, Wdoc *d)
{
	char tmp[40];

	sprint(tmp, "D%lud", d->time);
	s = s_append(s, tmp);
	if(d->comment){
		s = s_append(s, "\nC");
		s = s_append(s, d->comment);
	}
	if(d->author){
		s = s_append(s, "\nA");
		s = s_append(s, d->author);
	}
	if(d->conflict)
		s = s_append(s, "\nX");
	s = s_append(s, "\n");
	s = pagetext(s, d->wtxt, 1);
	return s;
}
