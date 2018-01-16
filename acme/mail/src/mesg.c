/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <u.h>
#include <libc.h>
#include <bio.h>
#include <thread.h>
#include <ctype.h>
#include <plumb.h>
#include "dat.h"

enum
{
	DIRCHUNK = 32*sizeof(Dir)
};

char	regexchars[] = "\\/[].+?()*^$";
char	deleted[] = "(deleted)-";
char	deletedrx[] = "\\(deleted\\)-";
char	deletedrx01[] = "(\\(deleted\\)-)?";
char	deletedaddr[] = "-#0;/^\\(deleted\\)-/";

struct{
	char	*type;
	char	*port;
	char *suffix;
} ports[] = {
	{"text/",			"edit",		".txt"},
	/* text must be first for plumbport() */
	{"image/gif",			"image",	".gif"},
	{"image/jpeg",			"image",	".jpg"},
	{"image/jpeg",			"image",	".jpeg"},
	{"image/png",			"image",	".png"},
	{"image/tiff",			"image",	".tif"},
	{"application/postscript",	"postscript",	".ps"},
	{"application/pdf",		"postscript",	".pdf"},
	{"application/msword",		"msword",	".doc"},
	{"application/rtf",		"msword",	".rtf"},
	{"audio/x-wav",			"wav",		".wav"},
	{nil,				nil,		nil}
};

char *goodtypes[] = {
	"text",
	"text/plain",
	"message/rfc822",
	"text/richtext",
	"text/tab-separated-values",
	"text/calendar",
	"application/octet-stream",
	nil,
};

struct{
	char *type;
	char	*ext;
} exts[] = {
	{"image/gif",	".gif"},
	{"image/jpeg",	".jpg"},
	{nil, nil}
};

char *okheaders[] =
{
	"From:",
	"Date:",
	"To:",
	"CC:",
	"Subject:",
	nil
};

char *extraheaders[] =
{
	"Resent-From:",
	"Resent-To:",
	"Sort:",
	nil,
};

char*
line(char *data, char **pp)
{
	char *p, *q;

	for(p=data; *p!='\0' && *p!='\n'; p++)
		;
	if(*p == '\n')
		*pp = p+1;
	else
		*pp = p;
	q = emalloc(p-data + 1);
	memmove(q, data, p-data);
	return q;
}

void
scanheaders(Message *m, char *dir)
{
	char *s, *t, *u, *f;

	s = f = readfile(dir, "header", nil);
	if(s != nil)
		while(*s){
			t = line(s, &s);
			if(strncmp(t, "From: ", 6) == 0){
				m->fromcolon = estrdup(t+6);
				/* remove all quotes; they're ugly and irregular */
				for(u=m->fromcolon; *u; u++)
					if(*u == '"')
						memmove(u, u+1, strlen(u));
			}
			if(strncmp(t, "Subject: ", 9) == 0)
				m->subject = estrdup(t+9);
			free(t);
		}
	if(m->fromcolon == nil)
		m->fromcolon = estrdup(m->from);
	free(f);
}

int
loadinfo(Message *m, char *dir)
{
	int n;
	char *data, *p, *s;

	data = readfile(dir, "info", &n);
	if(data == nil)
		return 0;
	m->from = line(data, &p);
	scanheaders(m, dir);	/* depends on m->from being set */
	m->to = line(p, &p);
	m->cc = line(p, &p);
	m->replyto = line(p, &p);
	m->date = line(p, &p);
	s = line(p, &p);
	if(m->subject == nil)
		m->subject = s;
	else
		free(s);
	m->type = line(p, &p);
	m->disposition = line(p, &p);
	m->filename = line(p, &p);
	m->digest = line(p, &p);
	free(data);
	return 1;
}

int
isnumeric(char *s)
{
	while(*s){
		if(!isdigit(*s))
			return 0;
		s++;
	}
	return 1;
}

Dir*
loaddir(char *name, int *np)
{
	int fd;
	Dir *dp;

	fd = open(name, OREAD);
	if(fd < 0)
		return nil;
	*np = dirreadall(fd, &dp);
	close(fd);
	return dp;
}

void
readmbox(Message *mbox, char *dir, char *subdir)
{
	char *name;
	Dir *d, *dirp;
	int i, n;

	name = estrstrdup(dir, subdir);
	dirp = loaddir(name, &n);
	mbox->recursed = 1;
	if(dirp)
		for(i=0; i<n; i++){
			d = &dirp[i];
			if(isnumeric(d->name))
				mesgadd(mbox, name, d, nil);
		}
	free(dirp);
	free(name);
}

/* add message to box, in increasing numerical order */
int
mesgadd(Message *mbox, char *dir, Dir *d, char *digest)
{
	Message *m;
	char *name;
	int loaded;

	m = emalloc(sizeof(Message));
	m->name = estrstrdup(d->name, "/");
	m->next = nil;
	m->prev = mbox->tail;
	m->level= mbox->level+1;
	m->recursed = 0;
	name = estrstrdup(dir, m->name);
	loaded = loadinfo(m, name);
	free(name);
	/* if two upas/fs are running, we can get misled, so check digest before accepting message */
	if(loaded==0 || (digest!=nil && m->digest!=nil && strcmp(digest, m->digest)!=0)){
		mesgfreeparts(m);
		free(m);
		return 0;
	}
	if(mbox->tail != nil)
		mbox->tail->next = m;
	mbox->tail = m;
	if(mbox->head == nil)
		mbox->head = m;

	if (m->level != 1){
		m->recursed = 1;
		readmbox(m, dir, m->name); 
	}
	return 1;
}

int
thisyear(char *year)
{
	static char now[10];
	char *s;

	if(now[0] == '\0'){
		s = ctime(time(nil));
		strcpy(now, s+24);
	}
	return strncmp(year, now, 4) == 0;
}

char*
stripdate(char *as)
{
	int n;
	char *s, *fld[10];

	as = estrdup(as);
	s = estrdup(as);
	n = tokenize(s, fld, 10);
	if(n > 5){
		sprint(as, "%.3s ", fld[0]);	/* day */
		/* some dates have 19 Apr, some Apr 19 */
		if(strlen(fld[1])<4 && isnumeric(fld[1]))
			sprint(as+strlen(as), "%.3s %.3s ", fld[1], fld[2]);	/* date, month */
		else
			sprint(as+strlen(as), "%.3s %.3s ", fld[2], fld[1]);	/* date, month */
		/* do we use time or year?  depends on whether year matches this one */
		if(thisyear(fld[5])){
			if(strchr(fld[3], ':') != nil)
				sprint(as+strlen(as), "%.5s ", fld[3]);	/* time */
			else if(strchr(fld[4], ':') != nil)
				sprint(as+strlen(as), "%.5s ", fld[4]);	/* time */
		}else
			sprint(as+strlen(as), "%.4s ", fld[5]);	/* year */
	}
	free(s);
	return as;
}

char*
readfile(char *dir, char *name, int *np)
{
	char *file, *data;
	int fd, len;
	Dir *d;

	if(np != nil)
		*np = 0;
	file = estrstrdup(dir, name);
	fd = open(file, OREAD);
	if(fd < 0)
		return nil;
	d = dirfstat(fd);
	free(file);
	len = 0;
	if(d != nil)
		len = d->length;
	free(d);
	data = emalloc(len+1);
	read(fd, data, len);
	close(fd);
	if(np != nil)
		*np = len;
	return data;
}

char*
info(Message *m, int ind, int ogf)
{
	char *i;
	int j, len, lens;
	char *p;
	char fmt[80], s[80];

	if (ogf)
		p=m->to;
	else
		p=m->fromcolon;

	if(ind==0 && shortmenu){
		len = 30;
		lens = 30;
		if(shortmenu > 1){
			len = 10;
			lens = 25;
		}
		if(ind==0 && m->subject[0]=='\0'){
			snprint(fmt, sizeof fmt, " %%-%d.%ds", len, len);
			snprint(s, sizeof s, fmt, p);
		}else{
			snprint(fmt, sizeof fmt, " %%-%d.%ds  %%-%d.%ds", len, len, lens, lens);
			snprint(s, sizeof s, fmt, p, m->subject);
		}
		i = estrdup(s);

		return i;
	} 

	i = estrdup("");
	i = eappend(i, "\t", p);
	i = egrow(i, "\t", stripdate(m->date));
	if(ind == 0){
		if(strcmp(m->type, "text")!=0 && strncmp(m->type, "text/", 5)!=0 && 
		   strncmp(m->type, "multipart/", 10)!=0)
			i = egrow(i, "\t(", estrstrdup(m->type, ")"));
	}else if(strncmp(m->type, "multipart/", 10) != 0)
		i = egrow(i, "\t(", estrstrdup(m->type, ")"));
	if(m->subject[0] != '\0'){
		i = eappend(i, "\n", nil);
		for(j=0; j<ind; j++)
			i = eappend(i, "\t", nil);
		i = eappend(i, "\t", m->subject);
	}
	return i;
}

void
mesgmenu0(Window *w, Message *mbox, char *realdir, char *dir, int ind, Biobuf *fd, int onlyone, int dotail)
{
	int i;
	Message *m;
	char *name, *tmp;
	int ogf=0;

	if(strstr(realdir, "outgoing") != nil)
		ogf=1;

	/* show mail box in reverse order, pieces in forward order */
	if(ind > 0)
		m = mbox->head;
	else
		m = mbox->tail;
	while(m != nil){
		for(i=0; i<ind; i++)
			Bprint(fd, "\t");
		if(ind != 0)
			Bprint(fd, "  ");
		name = estrstrdup(dir, m->name);
		tmp = info(m, ind, ogf);
		Bprint(fd, "%s%s\n", name, tmp);
		free(tmp);
		if(dotail && m->tail)
			mesgmenu0(w, m, realdir, name, ind+1, fd, 0, dotail);
		free(name);
		if(ind)
			m = m->next;
		else
			m = m->prev;
		if(onlyone)
			m = nil;
	}
}

void
mesgmenu(Window *w, Message *mbox)
{
	winopenbody(w, OWRITE);
	mesgmenu0(w, mbox, mbox->name, "", 0, w->body, 0, !shortmenu);
	winclosebody(w);
}

/* one new message has arrived, as mbox->tail */
void
mesgmenunew(Window *w, Message *mbox)
{
	Biobuf *b;

	winselect(w, "0", 0);
	w->data = winopenfile(w, "data");
	b = emalloc(sizeof(Biobuf));
	Binit(b, w->data, OWRITE);
	mesgmenu0(w, mbox, mbox->name, "", 0, b, 1, !shortmenu);
	Bterm(b);
	free(b);
	if(!mbox->dirty)
		winclean(w);
	/* select tag line plus following indented lines, but not final newline (it's distinctive) */
	winselect(w, "0/.*\\n((\t.*\\n)*\t.*)?/", 1);
	close(w->addr);
	close(w->data);
	w->addr = -1;
	w->data = -1;
}

char*
name2regexp(char *prefix, char *s)
{
	char *buf, *p, *q;

	buf = emalloc(strlen(prefix)+2*strlen(s)+50);	/* leave room to append more */
	p = buf;
	*p++ = '0';
	*p++ = '/';
	*p++ = '^';
	strcpy(p, prefix);
	p += strlen(prefix);
	for(q=s; *q!='\0'; q++){
		if(strchr(regexchars, *q) != nil)
			*p++ = '\\';
		*p++ = *q;
	}
	*p++ = '/';
	*p = '\0';
	return buf;
}

void
mesgmenumarkdel(Window *w, Message *mbox, Message *m, int writeback)
{
	char *buf;


	if(m->deleted)
		return;
	m->writebackdel = writeback;
	if(w->data < 0)
		w->data = winopenfile(w, "data");
	buf = name2regexp("", m->name);
	strcat(buf, "-#0");
	if(winselect(w, buf, 1))
		write(w->data, deleted, 10);
	free(buf);
	close(w->data);
	close(w->addr);
	w->addr = w->data = -1;
	mbox->dirty = 1;
	m->deleted = 1;
}

void
mesgmenumarkundel(Window *w, Message* _, Message *m)
{
	char *buf;

	if(m->deleted == 0)
		return;
	if(w->data < 0)
		w->data = winopenfile(w, "data");
	buf = name2regexp(deletedrx, m->name);
	if(winselect(w, buf, 1))
		if(winsetaddr(w, deletedaddr, 1))
			write(w->data, "", 0);
	free(buf);
	close(w->data);
	close(w->addr);
	w->addr = w->data = -1;
	m->deleted = 0;
}

void
mesgmenudel(Window *w, Message *mbox, Message *m)
{
	char *buf;

	if(w->data < 0)
		w->data = winopenfile(w, "data");
	buf = name2regexp(deletedrx, m->name);
	if(winsetaddr(w, buf, 1) && winsetaddr(w, ".,./.*\\n(\t.*\\n)*/", 1))
		write(w->data, "", 0);
	free(buf);
	close(w->data);
	close(w->addr);
	w->addr = w->data = -1;
	mbox->dirty = 1;
	m->deleted = 1;
}

void
mesgmenumark(Window *w, char *which, char *mark)
{
	char *buf;

	if(w->data < 0)
		w->data = winopenfile(w, "data");
	buf = name2regexp(deletedrx01, which);
	if(winsetaddr(w, buf, 1) && winsetaddr(w, "+0-#1", 1))	/* go to end of line */
		write(w->data, mark, strlen(mark));
	free(buf);
	close(w->data);
	close(w->addr);
	w->addr = w->data = -1;
	if(!mbox.dirty)
		winclean(w);
}

void
mesgfreeparts(Message *m)
{
	free(m->name);
	free(m->replyname);
	free(m->fromcolon);
	free(m->from);
	free(m->to);
	free(m->cc);
	free(m->replyto);
	free(m->date);
	free(m->subject);
	free(m->type);
	free(m->disposition);
	free(m->filename);
	free(m->digest);
}

void
mesgdel(Message *mbox, Message *m)
{
	Message *n, *next;

	if(m->opened)
		error("internal error: deleted message still open in mesgdel");
	/* delete subparts */
	for(n=m->head; n!=nil; n=next){
		next = n->next;
		mesgdel(m, n);
	}
	/* remove this message from list */
	if(m->next)
		m->next->prev = m->prev;
	else
		mbox->tail = m->prev;
	if(m->prev)
		m->prev->next = m->next;
	else
		mbox->head = m->next;

	mesgfreeparts(m);
}

int
mesgsave(Message *m, char *s)
{
	int ofd, n, k, ret;
	char *t, *raw, *unixheader, *all;

	t = estrstrdup(mbox.name, m->name);
	raw = readfile(t, "raw", &n);
	unixheader = readfile(t, "unixheader", &k);
	if(raw==nil || unixheader==nil){
		fprint(2, "Mail: can't read %s: %r\n", t);
		free(t);
		return 0;
	}
	free(t);

	all = emalloc(k+n+1);
	memmove(all, unixheader, k);
	memmove(all+k, raw, n);
	memmove(all+k+n, "\n", 1);
	n += k+1;
	free(unixheader);
	free(raw);
	ret = 1;
	s = estrdup(s);
	if(s[0] != '/')
		s = egrow(estrdup(mailboxdir), "/", s);
	ofd = open(s, OWRITE);
	if(ofd < 0){
		fprint(2, "Mail: can't open %s: %r\n", s);
		ret = 0;
	}else if(seek(ofd, 0LL, 2)<0 || write(ofd, all, n)!=n){
		fprint(2, "Mail: save failed: can't write %s: %r\n", s);
		ret = 0;
	}
	free(all);
	close(ofd);
	free(s);
	return ret;
}

int
mesgcommand(Message *m, char *cmd)
{
	char *s;
	char *args[10];
	int ok, ret, nargs;

	s = cmd;
	ret = 1;
	nargs = tokenize(s, args, nelem(args));
	if(nargs == 0)
		return 0;
	if(strcmp(args[0], "Post") == 0){
		mesgsend(m);
		goto Return;
	}
	if(strncmp(args[0], "Save", 4) == 0){
		if(m->isreply)
			goto Return;
		s = estrdup("\t[saved");
		if(nargs==1 || strcmp(args[1], "")==0){
			ok = mesgsave(m, "stored");
		}else{
			ok = mesgsave(m, args[1]);
			s = eappend(s, " ", args[1]);
		}
		if(ok){
			s = egrow(s, "]", nil);
			mesgmenumark(mbox.w, m->name, s);
		}
		free(s);
		goto Return;
	}
	if(strcmp(args[0], "Reply")==0){
		if(nargs>=2 && strcmp(args[1], "all")==0)
			mkreply(m, "Replyall", nil, nil, nil);
		else
			mkreply(m, "Reply", nil, nil, nil);
		goto Return;
	}
	if(strcmp(args[0], "Q") == 0){
		s = winselection(m->w);	/* will be freed by mkreply */
		if(nargs>=3 && strcmp(args[1], "Reply")==0 && strcmp(args[2], "all")==0)
			mkreply(m, "QReplyall", nil, nil, s);
		else
			mkreply(m, "QReply", nil, nil, s);
		goto Return;
	}
	if(strcmp(args[0], "Del") == 0){
		if(windel(m->w, 0)){
			chanfree(m->w->cevent);
			free(m->w);
			m->w = nil;
			if(m->isreply)
				delreply(m);
			else{
				m->opened = 0;
				m->tagposted = 0;
			}
			free(cmd);
			threadexits(nil);
		}
		goto Return;
	}
	if(strcmp(args[0], "Delmesg") == 0){
		if(!m->isreply){
			mesgmenumarkdel(wbox, &mbox, m, 1);
			free(cmd);	/* mesgcommand might not return */
			mesgcommand(m, estrdup("Del"));
			return 1;
		}
		goto Return;
	}
	if(strcmp(args[0], "UnDelmesg") == 0){
		if(!m->isreply && m->deleted)
			mesgmenumarkundel(wbox, &mbox, m);
		goto Return;
	}
//	if(strcmp(args[0], "Headers") == 0){
//		m->showheaders();
//		return True;
//	}

	ret = 0;

    Return:
	free(cmd);
	return ret;
}

void
mesgtagpost(Message *m)
{
	if(m->tagposted)
		return;
	wintagwrite(m->w, " Post", 5);
	m->tagposted = 1;
}

/* need to expand selection more than default word */
//#pragma varargck argpos eval 2

int32_t
eval(Window *w, char *s, ...)
{
	char buf[64];
	va_list arg;

	va_start(arg, s);
	vsnprint(buf, sizeof buf, s, arg);
	va_end(arg);

	if(winsetaddr(w, buf, 1)==0)
		return -1;

	if(pread(w->addr, buf, 24, 0) != 24)
		return -1;
	return strtol(buf, 0, 10);
}

int
isemail(char *s)
{
	int nat;

	nat = 0;
	for(; *s; s++)
		if(*s == '@')
			nat++;
		else if(!isalpha(*s) && !isdigit(*s) && !strchr("_.-+/", *s))
			return 0;
	return nat==1;
}

char addrdelim[] =  "/[ \t\\n<>()\\[\\]]/";
char*
expandaddr(Window *w, Event *e)
{
	char *s;
	int32_t q0, q1;

	if(e->q0 != e->q1)	/* cannot happen */
		return nil;

	q0 = eval(w, "#%d-%s", e->q0, addrdelim);
	if(q0 == -1)	/* bad char not found */
		q0 = 0;
	else			/* increment past bad char */
		q0++;

	q1 = eval(w, "#%d+%s", e->q0, addrdelim);
	if(q1 < 0){
		q1 = eval(w, "$");
		if(q1 < 0)
			return nil;
	}
	if(q0 >= q1)
		return nil;
	s = emalloc((q1-q0)*UTFmax+1);
	winread(w, q0, q1, s);
	return s;
}

int
replytoaddr(Window *w, Message *m, Event *e, char *s)
{
	int did;
	char *buf;
	Plumbmsg *pm;

	buf = nil;
	did = 0;
	if(e->flag & 2){
		/* autoexpanded; use our own bigger expansion */
		buf = expandaddr(w, e);
		if(buf == nil)
			return 0;
		s = buf;
	}
	if(isemail(s)){
		did = 1;
		pm = emalloc(sizeof(Plumbmsg));
		pm->src = estrdup("Mail");
		pm->dst = estrdup("sendmail");
		pm->data = estrdup(s);
		pm->ndata = -1;
		if(m->subject && m->subject[0]){
			pm->attr = emalloc(sizeof(Plumbattr));
			pm->attr->name = estrdup("Subject");
			if(tolower(m->subject[0]) != 'r' || tolower(m->subject[1]) != 'e' || m->subject[2] != ':')
				pm->attr->value = estrstrdup("Re: ", m->subject);
			else
				pm->attr->value = estrdup(m->subject);
			pm->attr->next = nil;
		}
		if(plumbsend(plumbsendfd, pm) < 0)
			fprint(2, "error writing plumb message: %r\n");
		plumbfree(pm);
	}
	free(buf);
	return did;
}


void
mesgctl(void *v)
{
	Message *m;
	Window *w;
	Event *e, *eq, *e2, *ea;
	int na, nopen, i, j;
	char *os, *s, *t, *buf;

	m = v;
	w = m->w;
	threadsetname("mesgctl");
	proccreate(wineventproc, w, STACK);
	for(;;){
		e = recvp(w->cevent);
		switch(e->c1){
		default:
		Unk:
			print("unknown message %c%c\n", e->c1, e->c2);
			break;

		case 'E':	/* write to body; can't affect us */
			break;

		case 'F':	/* generated by our actions; ignore */
			break;

		case 'K':	/* type away; we don't care */
		case 'M':
			switch(e->c2){
			case 'x':	/* mouse only */
			case 'X':
				ea = nil;
				eq = e;
				if(e->flag & 2){
					e2 = recvp(w->cevent);
					eq = e2;
				}
				if(e->flag & 8){
					ea = recvp(w->cevent);
					recvp(w->cevent);
					na = ea->nb;
				}else
					na = 0;
				if(eq->q1>eq->q0 && eq->nb==0){
					s = emalloc((eq->q1-eq->q0)*UTFmax+1);
					winread(w, eq->q0, eq->q1, s);
				}else
					s = estrdup(eq->b);
				if(na){
					t = emalloc(strlen(s)+1+na+1);
					sprint(t, "%s %s", s, ea->b);
					free(s);
					s = t;
				}
				if(!mesgcommand(m, s))	/* send it back */
					winwriteevent(w, e);
				break;

			case 'l':	/* mouse only */
			case 'L':
				buf = nil;
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
				os = s;
				nopen = 0;
				do{
					/* skip mail box name if present */
					if(strncmp(s, mbox.name, strlen(mbox.name)) == 0)
						s += strlen(mbox.name);
					if(strstr(s, "body") != nil){
						/* strip any known extensions */
						for(i=0; exts[i].ext!=nil; i++){
							j = strlen(exts[i].ext);
							if(strlen(s)>j && strcmp(s+strlen(s)-j, exts[i].ext)==0){
								s[strlen(s)-j] = '\0';
								break;
							}
						}
						if(strlen(s)>5 && strcmp(s+strlen(s)-5, "/body")==0)
							s[strlen(s)-4] = '\0';	/* leave / in place */
					}
					nopen += mesgopen(&mbox, mbox.name, s, m, 0, nil);
					while(*s!=0 && *s++!='\n')
						;
				}while(*s);
				if(nopen == 0 && e->c1 == 'L')
					nopen += replytoaddr(w, m, e, os);
				if(nopen == 0)
					winwriteevent(w, e);
				free(buf);
				break;

			case 'I':	/* modify away; we don't care */
			case 'D':
				mesgtagpost(m);
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

void
mesgline(Message *m, char *header, char *value)
{
	if(strlen(value) > 0)
		Bprint(m->w->body, "%s: %s\n", header, value);
}

int
isprintable(char *type)
{
	int i;

	for(i=0; goodtypes[i]!=nil; i++)
		if(strcmp(type, goodtypes[i])==0)
			return 1;
	return 0;
}

char*
ext(char *type)
{
	int i;

	for(i=0; exts[i].type!=nil; i++)
		if(strcmp(type, exts[i].type)==0)
			return exts[i].ext;
	return "";
}

void
mimedisplay(Message *m, char *name, char *rootdir, Window *w, int fileonly)
{
	char *dest, *maildest;

	if(strcmp(m->disposition, "file")==0 || strlen(m->filename)!=0){
		if(strlen(m->filename) == 0){
			dest = estrdup(m->name);
			dest[strlen(dest)-1] = '\0';
		}else
			dest = estrdup(m->filename);
		if((maildest = getenv("maildest")) != nil){
			maildest = eappend(maildest, "/", dest);
			Bprint(w->body, "\tcp %s%sbody%s %q\n", rootdir, name, ext(m->type), maildest);
			free(maildest);
		}
		if(m->filename[0] != '/'){
			dest = egrow(estrdup(home), "/", dest);
		}
		Bprint(w->body, "\tcp %s%sbody%s %q\n", rootdir, name, ext(m->type), dest);
		free(dest);
	}else if(!fileonly)
		Bprint(w->body, "\tfile is %s%sbody%s\n", rootdir, name, ext(m->type));
}

void
printheader(char *dir, Biobuf *b, char **okheaders)
{
	char *s;
	char *lines[100];
	int i, j, n;

	s = readfile(dir, "header", nil);
	if(s == nil)
		return;
	n = getfields(s, lines, nelem(lines), 0, "\n");
	for(i=0; i<n; i++)
		for(j=0; okheaders[j]; j++)
			if(cistrncmp(lines[i], okheaders[j], strlen(okheaders[j])) == 0)
				Bprint(b, "%s\n", lines[i]);
	free(s);
}

/*
 * find the best alternative part.
 *
 * turkeys have started emitting empty text/plain parts,
 * with the actual content in a text/html part, which complicates the choice.
 *
 * bigger turkeys emit a tiny base64-encoded text/plain part,
 * a small base64-encoded text/html part, and the real content is in
 * a text/calendar part.
 *
 * the magic lengths are empirically derived.
 * as turkeys devolve and mutate, this will only get worse.
 */
static Message*
bestalt(Message *m, char *dir)
{
	int len;
	char *subdir;
	Message *nm;
	Message *realplain, *realhtml, *realcal;

	realplain = realhtml = realcal = nil;
	for(nm = m->head; nm != nil; nm = nm->next){
		subdir = estrstrdup(dir, nm->name);
		len = 0;
		free(readbody(nm->type, subdir, &len));
		free(subdir);
		if(strcmp(nm->type, "text/plain") == 0 && len >= 8)
			realplain = nm;
		else if(strcmp(nm->type, "text/html") == 0 && len >= 600)
			realhtml = nm;
		else if(strcmp(nm->type, "text/calendar") == 0)
			realcal = nm;
	}
	if(realplain == nil && realhtml == nil && realcal)
		return realcal;			/* super-turkey */
	else if(realplain == nil && realhtml)
		return realhtml;		/* regular turkey */
	else
		return realplain;
}

void
mesgload(Message *m, char *rootdir, char *file, Window *w)
{
	char *s, *subdir, *name, *dir;
	Message *mp, *thisone;
	int n;

	dir = estrstrdup(rootdir, file);

	if(strcmp(m->type, "message/rfc822") != 0){	/* suppress headers of envelopes */
		if(strlen(m->from) > 0){
			Bprint(w->body, "From: %s\n", m->from);
			mesgline(m, "Date", m->date);
			mesgline(m, "To", m->to);
			mesgline(m, "CC", m->cc);
			mesgline(m, "Subject", m->subject);
			printheader(dir, w->body, extraheaders);
		}else{
			printheader(dir, w->body, okheaders);
			printheader(dir, w->body, extraheaders);
		}
		Bprint(w->body, "\n");
	}

	if(m->level == 1 && m->recursed == 0){
		m->recursed = 1;
		readmbox(m, rootdir, m->name);
	}
	if(m->head == nil){	/* single part message */
		if(strcmp(m->type, "text")==0 || strncmp(m->type, "text/", 5)==0){
			mimedisplay(m, m->name, rootdir, w, 1);
			s = readbody(m->type, dir, &n);
			winwritebody(w, s, n);
			free(s);
		}else
			mimedisplay(m, m->name, rootdir, w, 0);
	}else{
		/* multi-part message, either multipart or message/rfc822 */
		thisone = nil;
		if(strcmp(m->type, "multipart/alternative") == 0){
			thisone = bestalt(m, dir);
			if(thisone == nil){
				thisone = m->head; /* in case we can't find a good one */
				for(mp=m->head; mp!=nil; mp=mp->next)
					if(isprintable(mp->type)){
						thisone = mp;
						break;
					}
			}
		}
		for(mp=m->head; mp!=nil; mp=mp->next){
			if(thisone!=nil && mp!=thisone)
				continue;
			subdir = estrstrdup(dir, mp->name);
			name = estrstrdup(file, mp->name);
			/* skip first element in name because it's already in window name */
			if(mp != m->head)
				Bprint(w->body, "\n===> %s (%s) [%s]\n",
					strchr(name, '/')+1, mp->type,
					mp->disposition);
			if(strcmp(mp->type, "text")==0 ||
			    strncmp(mp->type, "text/", 5)==0){
				mimedisplay(mp, name, rootdir, w, 1);
				printheader(subdir, w->body, okheaders);
				printheader(subdir, w->body, extraheaders);
				winwritebody(w, "\n", 1);
				s = readbody(mp->type, subdir, &n);
				winwritebody(w, s, n);
				free(s);
			}else{
				if(strncmp(mp->type, "multipart/", 10)==0 ||
				    strcmp(mp->type, "message/rfc822")==0){
					mp->w = w;
					mesgload(mp, rootdir, name, w);
					mp->w = nil;
				}else
					mimedisplay(mp, name, rootdir, w, 0);
			}
			free(name);
			free(subdir);
		}
	}
	free(dir);
}

int
tokenizec(char *str, char **args, int max, char *splitc)
{
	int na;
	int intok = 0;

	if(max <= 0)
		return 0;	
	for(na=0; *str != '\0';str++){
		if(strchr(splitc, *str) == nil){
			if(intok)
				continue;
			args[na++] = str;
			intok = 1;
		}else{
			/* it's a separator/skip character */
			*str = '\0';
			if(intok){
				intok = 0;
				if(na >= max)
					break;
			}
		}
	}
	return na;
}

Message*
mesglookup(Message *mbox, char *name, char *digest)
{
	int n;
	Message *m;
	char *t;

	if(digest){
		/* can find exactly */
		for(m=mbox->head; m!=nil; m=m->next)
			if(strcmp(digest, m->digest) == 0)
				break;
		return m;
	}

	n = strlen(name);
	if(n == 0)
		return nil;
	if(name[n-1] == '/')
		t = estrdup(name);
	else
		t = estrstrdup(name, "/");
	for(m=mbox->head; m!=nil; m=m->next)
		if(strcmp(t, m->name) == 0)
			break;
	free(t);
	return m;
}

/*
 * Find plumb port, knowing type is text, given file name (by extension)
 */
int
plumbportbysuffix(char *file)
{
	char *suf;
	int i, nsuf, nfile;

	nfile = strlen(file);
	for(i=0; ports[i].type!=nil; i++){
		suf = ports[i].suffix;
		nsuf = strlen(suf);
		if(nfile > nsuf)
			if(cistrncmp(file+nfile-nsuf, suf, nsuf) == 0)
				return i;
	}
	return 0;
}

/*
 * Find plumb port using type and file name (by extension)
 */
int
plumbport(char *type, char *file)
{
	int i;

	for(i=0; ports[i].type!=nil; i++)
		if(strncmp(type, ports[i].type, strlen(ports[i].type)) == 0)
			return i;
	/* see if it's a text type */
	for(i=0; goodtypes[i]!=nil; i++)
		if(strncmp(type, goodtypes[i], strlen(goodtypes[i])) == 0)
			return plumbportbysuffix(file);
	return -1;
}

void
plumb(Message *m, char *dir)
{
	int i;
	char *port;
	Plumbmsg *pm;

	if(strlen(m->type) == 0)
		return;
	i = plumbport(m->type, m->filename);
	if(i < 0)
		fprint(2, "can't find destination for message subpart\n");
	else{
		port = ports[i].port;
		pm = emalloc(sizeof(Plumbmsg));
		pm->src = estrdup("Mail");
		if(port)
			pm->dst = estrdup(port);
		else
			pm->dst = nil;
		pm->wdir = nil;
		pm->type = estrdup("text");
		pm->ndata = -1;
		pm->data = estrstrdup(dir, "body");
		pm->data = eappend(pm->data, "", ports[i].suffix);
		if(plumbsend(plumbsendfd, pm) < 0)
			fprint(2, "error writing plumb message: %r\n");
		plumbfree(pm);
	}
}

int
mesgopen(Message *mbox, char *dir, char *s, Message *mesg, int plumbed, char *digest)
{
	char *t, *u, *v;
	Message *m;
	char *direlem[10];
	int i, ndirelem, reuse;

	/* find white-space-delimited first word */
	for(t=s; *t!='\0' && !isspace(*t); t++)
		;
	u = emalloc(t-s+1);
	memmove(u, s, t-s);
	/* separate it on slashes */
	ndirelem = tokenizec(u, direlem, nelem(direlem), "/");
	if(ndirelem <= 0){
    Error:
		free(u);
		return 0;
	}
	if(plumbed){
		write(wctlfd, "top", 3);
		write(wctlfd, "current", 7);
	}
	/* open window for message */
	m = mesglookup(mbox, direlem[0], digest);
	if(m == nil)
		goto Error;
	if(mesg!=nil && m!=mesg)	/* string looked like subpart but isn't part of this message */
		goto Error;
	if(m->opened == 0){
		if(m->w == nil){
			reuse = 0;
			m->w = newwindow();
		}else{
			reuse = 1;
			/* re-use existing window */
			if(winsetaddr(m->w, "0,$", 1)){
				if(m->w->data < 0)
					m->w->data = winopenfile(m->w, "data");
				write(m->w->data, "", 0);
			}
		}
		v = estrstrdup(mbox->name, m->name);
		winname(m->w, v);
		free(v);
		if(!reuse){
			if(m->deleted)
				wintagwrite(m->w, "Q Reply all UnDelmesg Save ", 2+6+4+10+5);
			else
				wintagwrite(m->w, "Q Reply all Delmesg Save ", 2+6+4+8+5);
		}
		threadcreate(mesgctl, m, STACK);
		winopenbody(m->w, OWRITE);
		mesgload(m, dir, m->name, m->w);
		winclosebody(m->w);
		winclean(m->w);
		m->opened = 1;
		if(ndirelem == 1){
			free(u);
			return 1;
		}
	}
	if(ndirelem == 1 && plumbport(m->type, m->filename) <= 0){
		/* make sure dot is visible */
		ctlprint(m->w->ctl, "show\n");
		return 0;
	}
	/* walk to subpart */
	dir = estrstrdup(dir, m->name);
	for(i=1; i<ndirelem; i++){
		m = mesglookup(m, direlem[i], digest);
		if(m == nil)
			break;
		dir = egrow(dir, m->name, nil);
	}
	if(m != nil && plumbport(m->type, m->filename) > 0)
		plumb(m, dir);
	free(dir);
	free(u);
	return 1;
}

void
rewritembox(Window *w, Message *mbox)
{
	Message *m, *next;
	char *deletestr, *t;
	int nopen;

	deletestr = estrstrdup("delete ", fsname);

	nopen = 0;
	for(m=mbox->head; m!=nil; m=next){
		next = m->next;
		if(m->deleted == 0)
			continue;
		if(m->opened){
			nopen++;
			continue;
		}
		if(m->writebackdel){
			/* messages deleted by plumb message are not removed again */
			t = estrdup(m->name);
			if(strlen(t) > 0)
				t[strlen(t)-1] = '\0';
			deletestr = egrow(deletestr, " ", t);
		}
		mesgmenudel(w, mbox, m);
		mesgdel(mbox, m);
	}
	if(write(mbox->ctlfd, deletestr, strlen(deletestr)) < 0)
		fprint(2, "Mail: warning: error removing mail message files: %r\n");
	free(deletestr);
	winselect(w, "0", 0);
	if(nopen == 0)
		winclean(w);
	mbox->dirty = 0;
}

/* name is a full file name, but it might not belong to us */
Message*
mesglookupfile(Message *mbox, char *name, char *digest)
{
	int k, n;

	k = strlen(name);
	n = strlen(mbox->name);
	if(k==0 || strncmp(name, mbox->name, n) != 0){
//		fprint(2, "Mail: message %s not in this mailbox\n", name);
		return nil;
	}
	return mesglookup(mbox, name+n, digest);
}
