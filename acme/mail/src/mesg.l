#include <alef.h>
#include <bio.h>
#include "dat.h"
#include "fns.h"

intern int ignored;

enum
{
	None,
	Unknown,
	Ignore,
	CC,
	From,
	ReplyTo,
	Sender,
	Subject,
	To,
	NHeaders = 200
};

aggr	Hdrs
{
	byte	*name;
	int	type;
};


Hdrs hdrs[NHeaders+1] = {
	{ "CC:",				CC },
	{ "From:",				From },
	{ "Reply-To:",			ReplyTo },
	{ "Sender:",			Sender },
	{ "Subject:",			Subject },
	{ "To:",				To },
};

int
StRnCmP(byte *s, byte *t, int n)
{
	int c, d;
	while(n > 0){
		c = *s++;
		d = *t++;
		--n;
		if(c != d){
			if('a'<=c && c<='z')
				c -= 'a'-'A';
			if('a'<=d && d<='z')
				d -= 'a'-'A';
			if(c != d)
				return c-d;
		}
	}
	return 0;
}

intern	byte*	hdrbuf;

void
ignore(void)
{
	Biobuf *b;
	byte *s;
	int i;

	ignored = True;
	b = Bopen("/mail/lib/ignore", OREAD);
	if(b == nil)
		return;
	for(i=0; hdrs[i].name; i++)
		;
	while((s = b->rdline('\n')) != nil){
		s[b->linelen()-1] = 0;
		hdrs[i].name = strdup(s);
		hdrs[i].type = Ignore;
		if(++i >= NHeaders){
			fprint(2, "/mail/lib/ignore has more than %d headers\n", NHeaders);
			break;
		}
	}
	b->term();
}

(int, byte*, int)
readhdr(Box *b)
{
	int i, j, n, m, type;
	byte *s, *t;

	rescue{
		return (0, nil, None);
	}
	if(!ignored)
		ignore();
	(n, s) = b->readline();
	if(n <= 0)
		raise;
	for(i=0; i<n; i++){
		j = s[i];
		if(i>0 && j == ':')
			break;
		if(j<'!' || '~'<j){
			b->unreadline();
			raise;
		}
	}
	type = Unknown;
	for(i=0; hdrs[i].name; i++){
		j = strlen(hdrs[i].name);
		if(StRnCmP(hdrs[i].name, s, j) == 0){
			type = hdrs[i].type;
			break;
		}
	}
	hdrbuf = realloc(hdrbuf, n+1);
	memmove(hdrbuf, s, n);
	hdrbuf[n] = 0;
	s = hdrbuf;
	/* scan for multiple sublines */
	for(;;){
		(m, t) = b->readline();
		if(m<=0 || (t[0]!=' ' && t[0]!='\t')){
			b->unreadline();
			break;
		}
		/* absorb */
		hdrbuf = realloc(hdrbuf, n+m+1);
		s = hdrbuf;
		memmove(s+n, t, m);
		n += m;
		s[n] = 0;
	}
	return(n, s, type);
}

Mesg*
Mesg.read(Box *b)
{
	Mesg *m;
	byte *s, *t;
	int n, type;

	(n, s) = b->readline();
	if(n <= 0)
		return nil;
	rescue{
		error("malformed header %s", s);
	}
	if(strncmp(s, "From ", 5) != 0)
		raise;
	m = malloc(sizeof(Mesg));
	m->realhdr = malloc(n+1);
	memmove(m->realhdr, s, n);
	m->lrealhdr = n;
	/* toss 'From ' */
	s += 5;
	n -= 5;
	m->hdr = malloc(n+1);
	memmove(m->hdr, s, n);
	/* convert first blank to tab */
	s = utfrune(m->hdr, ' ');
	if(s){
		*s = '\t';
		/* drop trailing seconds, time zone, and year if match local year */
		t = m->hdr+n-6;
		if(t <= m->hdr)
			raise;
		if(strcmp(t, date+23) == 0){
			strcpy(t, "\n");	/* drop year for sure */
			t = nil;
			s = utfrune(s, ':');
			if(s)
				t = utfrune(s+1, ':');
			if(t)	/* drop seconds and time zone */
				strcpy(t, "\n");
			else{	/* drop time zone */
				t = utfrune(s, ' ');
				if(t)
					strcpy(t, "\n");
			}
			n = strlen(m->hdr);
		}
	}
	m->lhdr = n;
	m->lline1 = n;
	m->text = malloc(1);
	m->ltext = 0;
	/* read header */
	for(;;){
		(n, s, type) = readhdr(b);
		switch(type){
		case None:
			break 2;

		case ReplyTo:
			m->replyto = realloc(m->replyto, n-9+1);
			memmove(m->replyto, s+9, n-9);
			m->replyto[n-9] = 0;
			break;

		case From:
			if(m->replyto == nil){
				m->replyto = realloc(m->replyto, n-5+1);
				memmove(m->replyto, s+5, n-5);
				m->replyto[n-5] = 0;
			}
			break;

		case Subject:
			m->subj = realloc(m->subj, n-8+1);
			memmove(m->subj, s+8, n-8);
			m->subj[n-8] = 0;
			break;
		}
		m->realhdr = realloc(m->realhdr, m->lrealhdr+n+1);
		memmove(m->realhdr+m->lrealhdr, s, n);
		m->lrealhdr += n;
		if(type != Ignore){
			m->hdr = realloc(m->hdr, m->lhdr+n+1);
			memmove(m->hdr+m->lhdr, s, n);
			m->lhdr += n;
			m->hdr[m->lhdr] = 0;
		}
	}
	/* read body */
	for(;;){
		(n, s) = b->readline();
		if(n <= 0)
			break;
		if(strncmp(s, "From ", 5) == 0){
			b->unreadline();
			break;
		}
		m->text = realloc(m->text, m->ltext+n+1);
		memmove(m->text+m->ltext, s, n);
		m->ltext += n;
	}
	/* remove trailing "morF\n" */
	if(m->ltext>6 && strncmp(m->text+m->ltext-6, "\nmorF\n", 6) == 0)
		m->ltext -= 5;
	m->text[m->ltext] = 0;
	m->box = b;
	return m;
}

void
Mesg.mkmail(Box *b, byte *hdr)
{
	Mesg *r;

	r = malloc(sizeof(Mesg));
	r->hdr = malloc(strlen(hdr)+2);
	strcpy(r->hdr, hdr);
	strcat(r->hdr, "\n");
	r->lhdr = strlen(hdr)+1;
	r->lline1 = r->lhdr;
	r->text = malloc(1);
	r->ltext = 0;
	r->box = b;
	r->open();
	r->wdormant();
}

byte*
replyaddr(byte *r)
{
	byte *p, *q;

	while(*r==' ' || *r=='\t')
		r++;
	r = strdup(r);
	p = utfrune(r, '<');
	if(p){
		p++;
		q = utfrune(p, '>');
		if(q == nil)
			q = p+strlen(p);
		else
			*q++ = '\n';
		memmove(r, p, q-p);
		r[q-p] = 0;
		return r;
	}
	p = utfrune(r, '(');
	if(p){
		q = utfrune(p, ')');
		if(q == nil)
			q = p+strlen(p);
		else
			q++;
		memmove(p, q, strlen(q)+1);
	}
	return r;
}

void
Mesg.mkreply(Mesg *m)
{
	Mesg *r;

	r = malloc(sizeof(Mesg));
	if(m->replyto){
		r->hdr = replyaddr(m->replyto);
		r->lhdr = strlen(r->hdr);
		r->lline1 = r->lhdr;
	}else{
		r->hdr = malloc(m->lline1+1);
		memmove(r->hdr, m->hdr, m->lline1);
		r->lhdr = m->lline1;
		r->lline1 = m->lhdr;
	}
	if(m->subj){
		r->ltext = 13+strlen(m->subj)+1;
		r->text = malloc(r->ltext+1);
		if(StRnCmP(m->subj, "re:", 3)==0 || StRnCmP(m->subj, " re:", 4)==0)
			sprint(r->text, "Subject:%s\n", m->subj);
		else
			sprint(r->text, "Subject: Re:%s\n", m->subj);
	}
	else{
		r->text = malloc(1);
		r->ltext = 0;
	}
	r->box = m->box;
	r->open();
	r->wselect("$");
	r->wdormant();
}

void
Mesg.free(Mesg *m)
{
	free(m->text);
	free(m->hdr);
	free(m->subj);
	free(m->realhdr);
	free(m->replyto);
	free(m);
}

Ref	replyid;

void
Mesg.open(Mesg *m)
{
	byte buf[256];
	byte *s;

	if(m->isopen)
		return;
	m->wnew();
	if(m->id != 0)
		m->wwritebody("From ", 5);
	m->wwritebody(m->hdr, m->lhdr);
	m->wwritebody(m->text, m->ltext);
	s = utfrrune(m->box->file, '/');
	if(s)
		s++;
	else
		s = m->box->file;
	if(m->id){
		sprint(buf, "Mail/%s/%d", s, m->id);
		m->wtagwrite("Reply Delmesg Save", 18);
	}else{
		sprint(buf, "Mail/%s/Reply%d", s, replyid.inc());
		m->wtagwrite("Post", 4);
	}
	m->wname(buf);
	m->wclean();
	m->wselect("0");
	m->isopen = True;
	m->posted = False;
	proc m->slave();
}

void
Mesg.putpost(Mesg *m, Event *e)
{
	if(m->posted || m->id==0)
		return;
	if(e->q0 >= m->lhdr+5)	/* include "From " */
		return;
	m->wtagwrite(" Post", 5);
	m->posted = True;
	return;
}

void
Mesg.slave(Mesg *m)
{
	Event e, e2, ea, etoss, *eq;
	byte *s, *t;
	int na;

	for(;;){
		m->wevent(&e);
		switch(e.c1){
		default:
		Unk:
			print("unknown message %c%c\n", e.c1, e.c2);
			break;

		case 'E':	/* write to body; can't affect us */
			break;

		case 'F':	/* generated by our actions; ignore */
			break;

		case 'K':	/* type away; we don't care */
		case 'M':
			switch(e.c2){
			case 'x':	/* mouse only */
			case 'X':
				eq = &e;
				if(e.flag & 2){
					m->wevent(&e2);
					eq = &e2;
				}
				if(e.flag & 8){
					m->wevent(&ea);
					m->wevent(&etoss);
					na = ea.nb;
				}else
					na = 0;
				if(eq->q1>eq->q0 && eq->nb==0){
					s = malloc((eq->q1-eq->q0)*UTFmax+1);
					m->wread(eq->q0, eq->q1, s);
				}else
					s = strdup(eq->b);
				if(na){
					t = malloc(strlen(s)+1+na+1);
					sprint(t, "%s %s", s, ea.b);
					free(s);
					s = t;
				}
				if(!m->command(s))	/* send it back */
					m->wwriteevent(&e);
				free(s);
				break;

			case 'l':	/* mouse only */
			case 'L':
				if(e.flag & 2)
					m->wevent(&e2);
				/* just send it back */
				m->wwriteevent(&e);
				break;

			case 'I':	/* modify away; we don't care */
			case 'D':
				m->putpost(&e);
				/* fall through */
			case 'd':
			case 'i':
				break;

			default:
				goto Unk;
			}
		}
	}
}

int
Mesg.command(Mesg *m, byte *s)
{
	byte *t;

	while(*s==' ' || *s=='\t' || *s=='\n')
		s++;
	if(strcmp(s, "Post") == 0){
		m->send();
		return True;
	}
	if(strncmp(s, "Save", 4) == 0){
		s += 4;
		while(*s==' ' || *s=='\t' || *s=='\n')
			s++;
		if(*s == 0)
			m->save("stored");
		else{
			t = s;
			while(*s && *s!=' ' && *s!='\t' && *s!='\n')
				s++;
			*s = 0;
			m->save(t);
		}
		return True;
	}
	if(strcmp(s, "Reply") == 0){
		m->mkreply();
		return True;
	}
	if(strcmp(s, "Del") == 0){
		if(m->wdel(False)){
			m->isopen = False;
			exits(nil);
		}
		return True;
	}
	if(strcmp(s, "Delmesg") == 0){
		if(m->wdel(False)){
			m->isopen = False;
			m->box->cdel <-= m;
			exits(nil);
		}
		return True;
	}
	return False;
}

void
Mesg.save(Mesg *m, byte *base)
{
	byte *s, *buf;
	int n, fd;
	Biobuf *b;

	if(m->id <= 0){
		fprint(2, "can't save reply message; mail it to yourself\n");
		return;
	}
	buf = nil;
	if(utfrune(base, '/'))
		s = base;
	else{
		buf = malloc(strlen(usermboxdir)+strlen(base)+1);
		sprint(buf, "%s%s", usermboxdir, base);
		s = buf;
	}
	rescue{
		if(buf)
			free(buf);
		fprint(2, "mail: can't open %s: %r\n", base);
		return;
	}
	if(access(s, 0) < 0)
		raise;
	fd = tryopen(s, OWRITE);
	if(fd < 0)
		raise;
	if(buf)
		free(buf);
	b = malloc(sizeof(Biobuf));
	b->init(fd, OWRITE);
	/* seek to end in case file isn't append-only */
	b->seek(0, 2);
	/* use edited headers: first line of real header followed by remainder of selected ones */
	for(n=0; n<m->lrealhdr && m->realhdr[n++]!='\n'; )
		;
	b->write(m->realhdr, n);
	b->write(m->hdr+m->lline1, m->lhdr-m->lline1);
	b->write(m->text, m->ltext);
	b->term();
	free(b);
	close(fd);
}

void
Mesg.send(Mesg *m)
{
	byte *as, *s, *t, *u, **a, buf[128];
	int n, na, p[2];
	chan(int) c;

	(nil, s) = m->wreadall();
	as = s;
	a = malloc(2*sizeof(byte*));
	a[0] = strdup("/bin/upas/sendmail");
	na = 1;
	if(strncmp(s, "From ", 5) == 0)
		s += 5;
	for(t=s; *t && *t!='\n' && *t!='\t';){
		while(*t==' ' || *t==',')
			t++;
		u = t;
		while(*t && *t!=' ' && *t!=',' && *t!='\t' && *t!='\n')
			t++;
		if(t == u)
			break;
		a = realloc(a, (na+2)*sizeof(byte*));
		n = t-u;
		a[na] = malloc(n+1);
		memmove(a[na], u, n);
		na++;
		a[na] = nil;
	}
	while(*t && *t!='\n')
		t++;
	if(*t == '\n')
		t++;
	if(pipe(p) < 0)
		error("can't pipe: %r");
	alloc c;
	proc run(a, c, p);
	<-c;
	unalloc c;
	close(p[0]);
	n = strlen(t);
	if(write(p[1], t, n) != n)
		fprint(2, "write to pipe failed: %r\n");
	close(p[1]);
	free(as);
	/* run() frees the arg list */
	s = utfrrune(m->box->file, '/');
	if(s)
		s++;
	else
		s = m->box->file;
	sprint(buf, "Mail/%s/%d-R", s, m->id);
	m->wname(buf);
	m->wclean();
}
