/*
 * Acme interface to nntpfs.
 */
#include <u.h>
#include <libc.h>
#include <bio.h>
#include <thread.h>
#include "win.h"
#include <ctype.h>

int canpost;
int debug;
int nshow = 20;

int lo;	/* next message to look at in dir */
int hi;	/* current hi message in dir */
char *dir = "/mnt/news";
char *group;
char *from;

typedef struct Article Article;
struct Article {
	Ref;
	Article *prev;
	Article *next;
	Window *w;
	int n;
	int dead;
	int dirtied;
	int sayspost;
	int headers;
	int ispost;
};

Article *mlist;
Window *root;

int
cistrncmp(char *a, char *b, int n)
{
	while(n-- > 0){
		if(tolower(*a++) != tolower(*b++))
			return -1;
	}
	return 0;
}

int
cistrcmp(char *a, char *b)
{
	for(;;){
		if(tolower(*a) != tolower(*b++))
			return -1;
		if(*a++ == 0)
			break;
	}
	return 0;
}

char*
skipwhite(char *p)
{
	while(isspace(*p))
		p++;
	return p;
}

int
gethi(void)
{
	Dir *d;
	int hi;

	if((d = dirstat(dir)) == nil)
		return -1;
	hi = d->qid.vers;
	free(d);
	return hi;
}

char*
fixfrom(char *s)
{
	char *r, *w;

	s = estrdup(s);
	/* remove quotes */
	for(r=w=s; *r; r++)
		if(*r != '"')
			*w++ = *r;
	*w = '\0';
	return s;
}

char *day[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", };
char *mon[] = {
	"Jan", "Feb", "Mar", "Apr",
	"May", "Jun", "Jul", "Aug",
	"Sep", "Oct", "Nov", "Dec",
};

char*
fixdate(char *s)
{
	char *f[10], *m, *t, *wd, tmp[40];
	int d, i, j, nf, hh, mm;

	nf = tokenize(s, f, nelem(f));

	wd = nil;
	d = 0;
	m = nil;
	t = nil;
	for(i=0; i<nf; i++){
		for(j=0; j<7; j++)
			if(cistrncmp(day[j], f[i], 3)==0)
				wd = day[j];
		for(j=0; j<12; j++)
			if(cistrncmp(mon[j], f[i], 3)==0)
				m = mon[j];
		j = atoi(f[i]);
		if(1 <= j && j <= 31 && d != 0)
			d = j;
		if(strchr(f[i], ':'))
			t = f[i];
	}

	if(d==0 || wd==nil || m==nil || t==nil)
		return nil;

	hh = strtol(t, 0, 10);
	mm = strtol(strchr(t, ':')+1, 0, 10);
	sprint(tmp, "%s %d %s %d:%.2d", wd, d, m, hh, mm);
	return estrdup(tmp);
}

void
msgheadline(Biobuf *bin, int n, Biobuf *bout)
{
	char *p, *q;
	char *date;
	char *from;
	char *subject;

	date = nil;
	from = nil;
	subject = nil;
	while(p = Brdline(bin, '\n')){
		p[Blinelen(bin)-1] = '\0';
		if((q = strchr(p, ':')) == nil)
			continue;
		*q++ = '\0';
		if(cistrcmp(p, "from")==0)
			from = fixfrom(skipwhite(q));
		else if(cistrcmp(p, "subject")==0)
			subject = estrdup(skipwhite(q));
		else if(cistrcmp(p, "date")==0)
			date = fixdate(skipwhite(q));
	}

	Bprint(bout, "%d/\t%s", n, from ? from : "");
	if(date)
		Bprint(bout, "\t%s", date);
	if(subject)
		Bprint(bout, "\n\t%s", subject);
	Bprint(bout, "\n");

	free(date);
	free(from);
	free(subject);
}

/*
 * Write the headers for at most nshow messages to b,
 * starting with hi and working down to lo.
 * Return number of first message not scanned.
 */
int
adddir(Biobuf *body, int hi, int lo, int nshow)
{
	char *p, *q, tmp[40];
	int i, n;
	Biobuf *b;

	n = 0;
	for(i=hi; i>=lo && n<nshow; i--){
		sprint(tmp, "%d", i);
		p = estrstrdup(dir, tmp);
		if(access(p, OREAD) < 0){
			free(p);
			break;
		}
		q = estrstrdup(p, "/header");
		free(p);
		b = Bopen(q, OREAD);
		free(q);
		if(b == nil)
			continue;
		msgheadline(b, i, body);
		Bterm(b);
		n++;
	}
	return i;
}

/* 
 * Show the first nshow messages in the window.
 * This depends on nntpfs presenting contiguously
 * numbered directories, and on the qid version being
 * the topmost numbered directory.
 */
void
dirwindow(Window *w)
{
	if((hi=gethi()) < 0)
		return;

	if(w->data < 0)
		w->data = winopenfile(w, "data");

	fprint(w->ctl, "dirty\n");
	
	winopenbody(w, OWRITE);
	lo = adddir(w->body, hi, 0, nshow);
	winclean(w);
}

/* return 1 if handled, 0 otherwise */
static int
iscmd(char *s, char *cmd)
{
	int len;

	len = strlen(cmd);
	return strncmp(s, cmd, len)==0 && (s[len]=='\0' || s[len]==' ' || s[len]=='\t' || s[len]=='\n');
}

static char*
skip(char *s, char *cmd)
{
	s += strlen(cmd);
	while(*s==' ' || *s=='\t' || *s=='\n')
		s++;
	return s;
}

void
unlink(Article *m)
{
	if(mlist == m)
		mlist = m->next;

	if(m->next)
		m->next->prev = m->prev;
	if(m->prev)
		m->prev->next = m->next;
	m->next = m->prev = nil;
}

int mesgopen(char*);
int fillmesgwindow(int, Article*);
Article *newpost(void);
void replywindow(Article*);
void mesgpost(Article*);

/*
 * Depends on d.qid.vers being highest numbered message in dir.
 */
void
acmetimer(Article *m, Window *w)
{
	Biobuf *b;
	Dir *d;

	assert(m==nil && w==root);

	if((d = dirstat(dir))==nil | hi==d->qid.vers){
		free(d);
		return;
	}

	if(w->data < 0)
		w->data = winopenfile(w, "data");
	if(winsetaddr(w, "0", 0))
		write(w->data, "", 0);

	b = emalloc(sizeof(*b));
	Binit(b, w->data, OWRITE);
	adddir(b, d->qid.vers, hi+1, d->qid.vers);
	hi = d->qid.vers;
	Bterm(b);
	free(b);
	free(d);
	winselect(w, "0,.", 0);
}

int
acmeload(Article*, Window*, char *s)
{
	int nopen;

//fprint(2, "load %s\n", s);

	nopen = 0;
	while(*s){
		/* skip directory name */
		if(strncmp(s, dir, strlen(dir))==0)
			s += strlen(dir);
		nopen += mesgopen(s);
		if((s = strchr(s, '\n')) == nil)
			break;
		s = skip(s, "");
	}
	return nopen;
}

int
acmecmd(Article *m, Window *w, char *s)
{
	int n;
	Biobuf *b;

//fprint(2, "cmd %s\n", s);

	s = skip(s, "");

	if(iscmd(s, "Del")){
		if(m == nil){	/* don't close dir until messages close */
			if(mlist != nil){
				ctlprint(mlist->w->ctl, "show\n");
				return 1;
			}
			if(windel(w, 0))
				threadexitsall(nil);
			return 1;
		}else{
			if(windel(w, 0))
				m->dead = 1;
			return 1;
		}
	}
	if(m==nil && iscmd(s, "More")){
		s = skip(s, "More");
		if(n = atoi(s))
			nshow = n;

		if(w->data < 0)
			w->data = winopenfile(w, "data");
		winsetaddr(w, "$", 1);
	
		fprint(w->ctl, "dirty\n");

		b = emalloc(sizeof(*b));
		Binit(b, w->data, OWRITE);
		lo = adddir(b, lo, 0, nshow);
		Bterm(b);
		free(b);		
		winclean(w);
		winsetaddr(w, ".,", 0);
	}
	if(m!=nil && !m->ispost && iscmd(s, "Headers")){
		m->headers = !m->headers;
		fillmesgwindow(-1, m);
		return 1;
	}
	if(iscmd(s, "Newpost")){
		m = newpost();
		winopenbody(m->w, OWRITE);
		Bprint(m->w->body, "%s\nsubject: \n\n", group);
		winclean(m->w);
		winselect(m->w, "$", 0);
		return 1;
	}
	if(m!=nil && !m->ispost && iscmd(s, "Reply")){
		replywindow(m);
		return 1;
	}
//	if(m!=nil && iscmd(s, "Replymail")){
//		fprint(2, "no replymail yet\n");
//		return 1;
//	}
	if(iscmd(s, "Post")){
		mesgpost(m);
		return 1;
	}
	return 0;
}

void
acmeevent(Article *m, Window *w, Event *e)
{
	Event *ea, *e2, *eq;
	char *s, *t, *buf;
	int na;
	//int n;
	//ulong q0, q1;

	switch(e->c1){	/* origin of action */
	default:
	Unknown:
		fprint(2, "unknown message %c%c\n", e->c1, e->c2);
		break;

	case 'T':	/* bogus timer event! */
		acmetimer(m, w);
		break;

	case 'F':	/* generated by our actions; ignore */
		break;

	case 'E':	/* write to body or tag; can't affect us */
		break;

	case 'K':	/* type away; we don't care */
		if(m && (e->c2 == 'I' || e->c2 == 'D')){
			m->dirtied = 1;
			if(!m->sayspost){
				wintagwrite(w, "Post ", 5);
				m->sayspost = 1;
			}
		}
		break;

	case 'M':	/* mouse event */
		switch(e->c2){		/* type of action */
		case 'x':	/* mouse: button 2 in tag */
		case 'X':	/* mouse: button 2 in body */
			ea = nil;
			//e2 = nil;
			s = e->b;
			if(e->flag & 2){	/* null string with non-null expansion */
				e2 = recvp(w->cevent);
				if(e->nb==0)
					s = e2->b;
			}
			if(e->flag & 8){	/* chorded argument */
				ea = recvp(w->cevent);	/* argument */
				na = ea->nb;
				recvp(w->cevent);		/* ignore origin */
			}else
				na = 0;
			
			/* append chorded arguments */
			if(na){
				t = emalloc(strlen(s)+1+na+1);
				sprint(t, "%s %s", s, ea->b);
				s = t;
			}
			/* if it's a known command, do it */
			/* if it's a long message, it can't be for us anyway */
		//	DPRINT(2, "exec: %s\n", s);
			if(!acmecmd(m, w, s))	/* send it back */
				winwriteevent(w, e);
			if(na)
				free(s);
			break;

		case 'l':	/* mouse: button 3 in tag */
		case 'L':	/* mouse: button 3 in body */
			//buf = nil;
			eq = e;
			if(e->flag & 2){
				e2 = recvp(w->cevent);
				eq = e2;
			}
			s = eq->b;
			if(eq->q1>eq->q0 && eq->nb==0){
				buf = emalloc((eq->q1-eq->q0)*UTFmax+1);
				winread(w, eq->q0, eq->q1, buf);
				s = buf;
			}
			if(!acmeload(m, w, s))
				winwriteevent(w, e);
			break;

		case 'i':	/* mouse: text inserted in tag */
		case 'd':	/* mouse: text deleted from tag */
			break;

		case 'I':	/* mouse: text inserted in body */
		case 'D':	/* mouse: text deleted from body */
			if(m == nil)
				break;

			m->dirtied = 1;
			if(!m->sayspost){
				wintagwrite(w, "Post ", 5);
				m->sayspost = 1;
			}
			break;

		default:
			goto Unknown;
		}
	}
}

void
dirthread(void *v)
{
	Event *e;
	Window *w;

	w = v;
	while(e = recvp(w->cevent))
		acmeevent(nil, w, e);

	threadexitsall(nil);
}

void
mesgthread(void *v)
{
	Event *e;
	Article *m;

	m = v;
	while(!m->dead && (e = recvp(m->w->cevent)))
		acmeevent(m, m->w, e);

//fprint(2, "msg %p exits\n", m);
	unlink(m);
	free(m->w);
	free(m);
	threadexits(nil);
}

/*
Xref: news.research.att.com comp.os.plan9:7360
Newsgroups: comp.os.plan9
Path: news.research.att.com!batch0!uunet!ffx.uu.net!finch!news.mindspring.net!newsfeed.mathworks.com!fu-berlin.de!server1.netnews.ja.net!hgmp.mrc.ac.uk!pegasus.csx.cam.ac.uk!bath.ac.uk!ccsdhd
From: Stephen Adam <saadam@bigpond.com>
Subject: Future of Plan9
Approved: plan9mod@bath.ac.uk
X-Newsreader: Microsoft Outlook Express 5.00.2014.211
X-Mimeole: Produced By Microsoft MimeOLE V5.00.2014.211
Sender: ccsdhd@bath.ac.uk (Dennis Davis)
Nntp-Posting-Date: Wed, 13 Dec 2000 21:28:45 EST
NNTP-Posting-Host: 203.54.121.233
Organization: Telstra BigPond Internet Services (http://www.bigpond.com)
X-Date: Wed, 13 Dec 2000 20:43:37 +1000
Lines: 12
Message-ID: <xbIZ5.157945$e5.114349@newsfeeds.bigpond.com>
References: <95pghu$3lf$1@news.fas.harvard.edu> <95ph36$3m9$1@news.fas.harvard.edu> <slrn980iic.u5q.mperrin@hcs.harvard.edu> <95png6$4ln$1@news.fas.harvard.edu> <95poqg$4rq$1@news.fas.harvard.edu> <slrn980vh8.2gb.myLastName@is07.fas.harvard.edu> <95q40h$66c$2@news.fas.harvard.edu> <95qjhu$8ke$1@news.fas.harvard.edu> <95riue$bu2$1@news.fas.harvard.edu> <95rnar$cbu$1@news.fas.harvard.edu>
X-Msmail-Priority: Normal
X-Trace: newsfeeds.bigpond.com 976703325 203.54.121.233 (Wed, 13 Dec 2000 21:28:45 EST)
X-Priority: 3
Date: Wed, 13 Dec 2000 10:49:50 GMT
*/

char *skipheader[] = 
{
	"x-",
	"path:",
	"xref:",
	"approved:",
	"sender:",
	"nntp-",
	"organization:",
	"lines:",
	"message-id:",
	"references:",
	"reply-to:",
	"mime-",
	"content-",
};

int
fillmesgwindow(int fd, Article *m)
{
	Biobuf *b;
	char *p, tmp[40];
	int i, inhdr, copy, xfd;
	Window *w;

	xfd = -1;
	if(fd == -1){
		sprint(tmp, "%d/article", m->n);
		p = estrstrdup(dir, tmp);
		if((xfd = open(p, OREAD)) < 0){
			free(p);	
			return 0;
		}
		free(p);
		fd = xfd;
	}

	w = m->w;
	if(w->data < 0)
		w->data = winopenfile(w, "data");
	if(winsetaddr(w, ",", 0))
		write(w->data, "", 0);

	winopenbody(m->w, OWRITE);
	b = emalloc(sizeof(*b));
	Binit(b, fd, OREAD);

	inhdr = 1;
	copy = 1;
	while(p = Brdline(b, '\n')){
		if(Blinelen(b)==1)
			inhdr = 0, copy=1;
		if(inhdr && !isspace(p[0])){
			copy = 1;
			if(!m->headers){
				if(cistrncmp(p, "from:", 5)==0){
					p[Blinelen(b)-1] = '\0';
					p = fixfrom(skip(p, "from:"));
					Bprint(m->w->body, "From: %s\n", p);
					free(p);
					copy = 0;
					continue;
				}
				for(i=0; i<nelem(skipheader); i++)
					if(cistrncmp(p, skipheader[i], strlen(skipheader[i]))==0)
						copy=0;
			}
		}
		if(copy)
			Bwrite(m->w->body, p, Blinelen(b));
	}
	Bterm(b);
	free(b);
	winclean(m->w);
	if(xfd != -1)
		close(xfd);
	return 1;
}

Article*
newpost(void)
{
	Article *m;
	char *p, tmp[40];
	static int nnew;

	m = emalloc(sizeof *m);
	sprint(tmp, "Post%d", ++nnew);
	p = estrstrdup(dir, tmp);

	m->w = newwindow();
	proccreate(wineventproc, m->w, STACK);
	winname(m->w, p);
	wintagwrite(m->w, "Post ", 5);
	m->sayspost = 1;
	m->ispost = 1;
	threadcreate(mesgthread, m, STACK);

	if(mlist){
		m->next = mlist;
		mlist->prev = m;
	}
	mlist = m;
	return m;
}

void
replywindow(Article *m)
{
	Biobuf *b;
	char *p, *ep, *q, tmp[40];
	int fd, copy;
	Article *reply;

	sprint(tmp, "%d/article", m->n);
	p = estrstrdup(dir, tmp);
	if((fd = open(p, OREAD)) < 0){
		free(p);	
		return;
	}
	free(p);

	reply = newpost();
	winopenbody(reply->w, OWRITE);
	b = emalloc(sizeof(*b));
	Binit(b, fd, OREAD);
	copy = 0;
	while(p = Brdline(b, '\n')){
		if(Blinelen(b)==1)
			break;
		ep = p+Blinelen(b);
		if(!isspace(*p)){
			copy = 0;
			if(cistrncmp(p, "newsgroups:", 11)==0){
				for(q=p+11; *q!='\n'; q++)
					if(*q==',')
						*q = ' ';
				copy = 1;
			}else if(cistrncmp(p, "subject:", 8)==0){
				if(!strstr(p, " Re:") && !strstr(p, " RE:") && !strstr(p, " re:")){
					p = skip(p, "subject:");
					ep[-1] = '\0';
					Bprint(reply->w->body, "Subject: Re: %s\n", p);
				}else
					copy = 1;
			}else if(cistrncmp(p, "message-id:", 11)==0){
				Bprint(reply->w->body, "References: ");
				p += 11;
				copy = 1;
			}
		}
		if(copy)
			Bwrite(reply->w->body, p, ep-p);
	}
	Bterm(b);
	close(fd);
	free(b);
	Bprint(reply->w->body, "\n");
	winclean(reply->w);
	winselect(reply->w, "$", 0);
}

char*
skipbl(char *s, char *e)
{
	while(s < e){
		if(*s!=' ' && *s!='\t' && *s!=',')
			break;
		s++;
	}
	return s;
}

char*
findbl(char *s, char *e)
{
	while(s < e){
		if(*s==' ' || *s=='\t' || *s==',')
			break;
		s++;
	}
	return s;
}

/*
 * comma-separate possibly blank-separated strings in line; e points before newline
 */
void
commas(char *s, char *e)
{
	char *t;

	/* may have initial blanks */
	s = skipbl(s, e);
	while(s < e){
		s = findbl(s, e);
		if(s == e)
			break;
		t = skipbl(s, e);
		if(t == e)	/* no more words */
			break;
		/* patch comma */
		*s++ = ',';
		while(s < t)
			*s++ = ' ';
	}
}
void
mesgpost(Article *m)
{
	Biobuf *b;
	char *p, *ep;
	int isfirst, ishdr, havegroup, havefrom;

	p = estrstrdup(dir, "post");
	if((b = Bopen(p, OWRITE)) == nil){
		fprint(2, "cannot open %s: %r\n", p);
		free(p);
		return;
	}
	free(p);

	winopenbody(m->w, OREAD);
	ishdr = 1;
	isfirst = 1;
	havegroup = havefrom = 0;
	while(p = Brdline(m->w->body, '\n')){
		ep = p+Blinelen(m->w->body);
		if(ishdr && p+1==ep){
			if(!havegroup)
				Bprint(b, "Newsgroups: %s\n", group);
			if(!havefrom)
				Bprint(b, "From: %s\n", from);
			ishdr = 0;
		}
		if(ishdr){
			ep[-1] = '\0';
			if(isfirst && strchr(p, ':')==0){	/* group list */
				commas(p, ep);
				Bprint(b, "newsgroups: %s\n", p);
				havegroup = 1;
				isfirst = 0;
				continue;
			}
			if(cistrncmp(p, "newsgroup:", 10)==0){
				commas(skip(p, "newsgroup:"), ep);
				Bprint(b, "newsgroups: %s\n", skip(p, "newsgroup:"));
				havegroup = 1;
				continue;
			}
			if(cistrncmp(p, "newsgroups:", 11)==0){
				commas(skip(p, "newsgroups:"), ep);
				Bprint(b, "newsgroups: %s\n", skip(p, "newsgroups:"));
				havegroup = 1;
				continue;
			}
			if(cistrncmp(p, "from:", 5)==0)
				havefrom = 1;
			ep[-1] = '\n';
		}
		Bwrite(b, p, ep-p);
	}
	winclosebody(m->w);
	Bflush(b);
	if(write(Bfildes(b), "", 0) == 0)
		winclean(m->w);
	else
		fprint(2, "post: %r\n");
	Bterm(b);
}

int
mesgopen(char *s)
{
	char *p, tmp[40];
	int fd, n;
	Article *m;

	n = atoi(s);
	if(n==0)
		return 0;

	for(m=mlist; m; m=m->next){
		if(m->n == n){
			ctlprint(m->w->ctl, "show\n");
			return 1;
		}
	}

	sprint(tmp, "%d/article", n);
	p = estrstrdup(dir, tmp);
	if((fd = open(p, OREAD)) < 0){
		free(p);	
		return 0;
	}

	m = emalloc(sizeof(*m));
	m->w = newwindow();
	m->n = n;
	proccreate(wineventproc, m->w, STACK);
	p[strlen(p)-strlen("article")] = '\0';
	winname(m->w, p);
	if(canpost)
		wintagwrite(m->w, "Reply ", 6);
	wintagwrite(m->w, "Headers ", 8);

	free(p);
	if(mlist){
		m->next = mlist;
		mlist->prev = m;
	}
	mlist = m;
	threadcreate(mesgthread, m, STACK);

	fillmesgwindow(fd, m);
	close(fd);
	windormant(m->w);
	return 1;
}

void
usage(void)
{
	fprint(2, "usage: News [-d /mnt/news] comp.os.plan9\n");
	exits("usage");
}

void
timerproc(void *v)
{
	Event e;
	Window *w;

	memset(&e, 0, sizeof e);
	e.c1 = 'T';
	w = v;

	for(;;){
		sleep(60*1000);
		sendp(w->cevent, &e);
	}
}

char*
findfrom(void)
{
	char *p, *u;
	Biobuf *b;

	u = getuser();
	if(u==nil)
		return "glenda";

	p = estrstrstrdup("/usr/", u, "/lib/newsfrom");
	b = Bopen(p, OREAD);
	free(p);
	if(b){
		p = Brdline(b, '\n');
		if(p){
			p[Blinelen(b)-1] = '\0';
			p = estrdup(p);
			Bterm(b);
			return p;
		}
		Bterm(b);
	}

	p = estrstrstrdup("/mail/box/", u, "/headers");
	b = Bopen(p, OREAD);
	free(p);
	if(b){
		while(p = Brdline(b, '\n')){
			p[Blinelen(b)-1] = '\0';
			if(cistrncmp(p, "from:", 5)==0){
				p = estrdup(skip(p, "from:"));
				Bterm(b);
				return p;
			}
		}
		Bterm(b);
	}

	return u;
}

void
threadmain(int argc, char **argv)
{
	char *p, *q;
	Dir *d;
	Window *w;

	ARGBEGIN{
	case 'D':
		debug++;
		break;
	case 'd':
		dir = EARGF(usage());
		break;
	default:
		usage();
		break;
	}ARGEND

	if(argc != 1)
		usage();

	from = findfrom();

	group = estrdup(argv[0]);	/* someone will be cute */
	while(q=strchr(group, '/'))
		*q = '.';

	p = estrdup(argv[0]);
	while(q=strchr(p, '.'))
		*q = '/';
	p = estrstrstrdup(dir, "/", p);
	cleanname(p);

	if((d = dirstat(p)) == nil){	/* maybe it is a new group */
		if((d = dirstat(dir)) == nil){
			fprint(2, "dirstat(%s) fails: %r\n", dir);
			threadexitsall(nil);
		}
		if((d->mode&DMDIR)==0){
			fprint(2, "%s not a directory\n", dir);
			threadexitsall(nil);
		}
		free(d);
		if((d = dirstat(p)) == nil){
			fprint(2, "stat %s: %r\n", p);
			threadexitsall(nil);
		}
	}
	if((d->mode&DMDIR)==0){
		fprint(2, "%s not a directory\n", dir);
		threadexitsall(nil);
	}
	free(d);
	dir = estrstrdup(p, "/");

	q = estrstrdup(dir, "post");
	canpost = access(q, AWRITE)==0;

	w = newwindow();
	root = w;
	proccreate(wineventproc, w, STACK);
	proccreate(timerproc, w, STACK);

	winname(w, dir);
	if(canpost)
		wintagwrite(w, "Newpost ", 8);
	wintagwrite(w, "More ", 5);
	dirwindow(w);
	threadcreate(dirthread, w, STACK);
	threadexits(nil);
}
