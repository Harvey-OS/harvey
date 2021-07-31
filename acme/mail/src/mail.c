#include <u.h>
#include <libc.h>
#include <bio.h>
#include <thread.h>
#include <plumb.h>
#include "dat.h"

char	*maildir = "/mail/fs/";			/* mountpoint of mail file system */
char	*mailtermdir = "/mnt/term/mail/fs/";	/* alternate mountpoint */
char *mboxname = "mbox";			/* mailboxdir/mboxname is mail spool file */
char	*mailboxdir = nil;				/* nil == /mail/box/$user */
char *fsname;						/* filesystem for mailboxdir/mboxname is at maildir/fsname */

Window	*wbox;
Message	mbox;
Message	replies;
char		*home;
int		plumbsendfd;
int		plumbseemailfd;
int		plumbshowmailfd;
Channel	*cplumb;
Channel	*cplumbshow;
int		wctlfd;
void		mainctl(void*);
void		plumbproc(void*);
void		plumbshowproc(void*);
void		plumbthread(void);
void		plumbshowthread(void*);

void
usage(void)
{
	threadprint(2, "usage: Mail [mailboxname [directoryname]]\n");
	threadexitsall("usage");
}

void
removeupasfs(void)
{
	char buf[256];

	if(strcmp(mboxname, "mbox") == 0)
		return;
	snprint(buf, sizeof buf, "close %s", mboxname);
	write(mbox.ctlfd, buf, strlen(buf));
}

void
threadmain(int argc, char *argv[])
{
	char *s, *user, *name;
	char err[ERRLEN], cmd[256];
	int i;

	/* open these early so we won't miss notification of new mail messages while we read mbox */
	plumbsendfd = plumbopen("send", OWRITE|OCEXEC);
	plumbseemailfd = plumbopen("seemail", OREAD|OCEXEC);
	plumbshowmailfd = plumbopen("showmail", OREAD|OCEXEC);

	ARGBEGIN{
	}ARGEND
	name = "mbox";
	if(argc > 0){
		i = strlen(argv[0]);
		if(argc>2 || i==0)
			usage();
		if(argv[0][i-1] == '/')
			argv[0][i-1] = '\0';
		s = strrchr(argv[0], '/');
		if(s == nil)
			mboxname = estrdup(argv[0]);
		else{
			*s++ = '\0';
			if(*s == '\0')
				usage();
			mailboxdir = argv[0];
			mboxname = estrdup(s);
		}
		if(argc > 1)
			name = argv[1];
		else
			name = mboxname;
	}

	user = getenv("user");
	if(user == nil)
		user = "none";
	if(mailboxdir == nil)
		mailboxdir = estrstrdup("/mail/box/", user);

	if(access(maildir, 0)<0 && access(mailtermdir, 0)==0)
		bind(mailtermdir, maildir, MAFTER);

	s = estrstrdup(maildir, "ctl");
	mbox.ctlfd = open(s, ORDWR|OCEXEC);
	if(mbox.ctlfd < 0)
		error("Mail: can't open %s: %r\n", s);

	fsname = estrdup(name);
	if(argc > 0){
		s = emalloc(5+strlen(mailboxdir)+strlen(mboxname)+strlen(name)+10+1);
		for(i=0; i<10; i++){
			sprint(s, "open %s/%s %s", mailboxdir, mboxname, fsname);
			if(write(mbox.ctlfd, s, strlen(s)) >= 0)
				break;
			err[0] = '\0';
			errstr(err);
			if(strcmp(err, "mbox name in use") != 0)
				error("Mail: can't create directory %s for mail: %s\n", name, err);
			free(fsname);
			fsname = emalloc(strlen(name)+10);
			sprint(fsname, "%s-%d", name, i);
		}
		free(s);
	}

	s = estrstrdup(fsname, "/");
	mbox.name = estrstrdup(maildir, s);
	readmbox(&mbox, maildir, s);
	home = getenv("home");
	if(home == nil)
		home = "/";

	wbox = newwindow();
	winname(wbox, mbox.name);
	wintagwrite(wbox, "Put Mail", 4+4);
	threadcreate(mainctl, wbox, STACK);
	snprint(cmd, sizeof cmd, "Mail %s", name);
	winsetdump(wbox, "/acme/mail", cmd);
	mbox.w = wbox;

	mesgmenu(wbox, &mbox);
	winclean(wbox);

	wctlfd = open("/dev/wctl", OWRITE|OCEXEC);	/* for acme window */
	cplumb = chancreate(sizeof(Plumbmsg*), 0);
	cplumbshow = chancreate(sizeof(Plumbmsg*), 0);
	/* start plumb reader as separate proc ... */
	proccreate(plumbproc, nil, STACK);
	proccreate(plumbshowproc, nil, STACK);
	threadcreate(plumbshowthread, nil, STACK);
	/* ... and use this thread to read the messages */
	plumbthread();
}

void
plumbproc(void*)
{
	Plumbmsg *m;

	threadsetname("plumbproc");
	for(;;){
		m = plumbrecv(plumbseemailfd);
		sendp(cplumb, m);
		if(m == nil)
			threadexits(nil);
	}
}

void
plumbshowproc(void*)
{
	Plumbmsg *m;

	threadsetname("plumbshowproc");
	for(;;){
		m = plumbrecv(plumbshowmailfd);
		sendp(cplumbshow, m);
		if(m == nil)
			threadexits(nil);
	}
}

void
newmesg(char *name, char *digest)
{
	Dir d;

	if(strncmp(name, mbox.name, strlen(mbox.name)) != 0)
		return;	/* message is about another mailbox */
	if(mesglookupfile(&mbox, name, digest) != nil)
		return;
	if(dirstat(name, &d) < 0)
		return;
	if(mesgadd(&mbox, mbox.name, &d, digest))
		mesgmenunew(wbox, &mbox);
}

void
showmesg(char *name, char *digest)
{
	char *n;

	if(strncmp(name, mbox.name, strlen(mbox.name)) != 0)
		return;	/* message is about another mailbox */
	n = estrdup(name+strlen(mbox.name));
	if(n[strlen(n)-1] != '/')
		n = egrow(n, "/", nil);
	mesgopen(&mbox, mbox.name, name+strlen(mbox.name), nil, 1, digest);
	free(n);
}

void
delmesg(char *name, char *digest)
{
	Message *m;

//threadprint(2, "Mail: ignoring delete message for %s\n", name);
	m = mesglookupfile(&mbox, name, digest);
	if(m != nil)
		mesgmenumarkdel(wbox, &mbox, m, 0);
}

void
plumbthread(void)
{
	Plumbmsg *m;
	Plumbattr *a;
	char *type, *digest;

	threadsetname("plumbthread");
	while((m = recvp(cplumb)) != nil){
		a = m->attr;
		digest = plumblookup(a, "digest");
		type = plumblookup(a, "mailtype");
		if(type == nil)
			threadprint(2, "Mail: plumb message with no mailtype attribute\n");
		else if(strcmp(type, "new") == 0)
			newmesg(m->data, digest);
		else if(strcmp(type, "delete") == 0)
			delmesg(m->data, digest);
		else
			threadprint(2, "Mail: unknown plumb attribute %s\n", type);
		plumbfree(m);
	}
	threadexits(nil);
}

void
plumbshowthread(void*)
{
	Plumbmsg *m;

	threadsetname("plumbshowthread");
	while((m = recvp(cplumbshow)) != nil){
		showmesg(m->data, plumblookup(m->attr, "digest"));
		plumbfree(m);
	}
	threadexits(nil);
}

int
mboxcommand(Window *w, char *s)
{
	char *t;
	Message *m, *next;
	int ok;

	while(*s==' ' || *s=='\t' || *s=='\n')
		s++;
	if(strncmp(s, "Mail", 4) == 0){
		s += 4;
		while(*s==' ' || *s=='\t' || *s=='\n')
			s++;
		t = s;
		while(*s && *s!=' ' && *s!='\t' && *s!='\n')
			s++;
		*s = 0;
		mkreply(nil, "Mail", t);
		return 1;
	}
	if(strcmp(s, "Del") == 0){
		if(mbox.dirty){
			mbox.dirty = 0;
			threadprint(2, "mail: mailbox not written\n");
			return 1;
		}
		ok = 1;
		for(m=mbox.head; m!=nil; m=next){
			next = m->next;
			if(m->w){
				if(windel(m->w, 0))
					m->w = nil;
				else
					ok = 0;
			}
		}
		for(m=replies.head; m!=nil; m=next){
			next = m->next;
			if(m->w){
				if(windel(m->w, 0))
					m->w = nil;
				else
					ok = 0;
			}
		}
		if(ok){
			windel(w, 1);
			removeupasfs();
			threadexitsall(nil);
		}
		return 1;
	}
	if(strcmp(s, "Put") == 0){
		rewritembox(wbox, &mbox);
		return 1;
	}
	return 0;
}

void
mainctl(void *v)
{
	Window *w;
	Event *e, *e2, *eq, *ea;
	int na, nopen;
	char *s, *t, *buf;

	w = v;
	proccreate(wineventproc, w, STACK);

	for(;;){
		e = recvp(w->cevent);
		switch(e->c1){
		default:
		Unknown:
			print("unknown message %c%c\n", e->c1, e->c2);
			break;
	
		case 'E':	/* write to body; can't affect us */
			break;
	
		case 'F':	/* generated by our actions; ignore */
			break;
	
		case 'K':	/* type away; we don't care */
			break;
	
		case 'M':
			switch(e->c2){
			case 'x':
			case 'X':
				ea = nil;
				e2 = nil;
				if(e->flag & 2)
					e2 = recvp(w->cevent);
				if(e->flag & 8){
					ea = recvp(w->cevent);
					na = ea->nb;
					recvp(w->cevent);
				}else
					na = 0;
				s = e->b;
				/* if it's a known command, do it */
				if((e->flag&2) && e->nb==0)
					s = e2->b;
				if(na){
					t = emalloc(strlen(s)+1+na+1);
					sprint(t, "%s %s", s, ea->b);
					s = t;
				}
				/* if it's a long message, it can't be for us anyway */
				if(!mboxcommand(w, s))	/* send it back */
					winwriteevent(w, e);
				if(na)
					free(s);
				break;
	
			case 'l':
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
				nopen = 0;
				do{
					/* skip 'deleted' string if present' */
					if(strncmp(s, deleted, strlen(deleted)) == 0)
						s += strlen(deleted);
					/* skip mail box name if present */
					if(strncmp(s, mbox.name, strlen(mbox.name)) == 0)
						s += strlen(mbox.name);
					nopen += mesgopen(&mbox, mbox.name, s, nil, 0, nil);
					while(*s!='\0' && *s++!='\n')
						;
				}while(*s);
				if(nopen == 0)	/* send it back */
					winwriteevent(w, e);
				free(buf);
				break;
	
			case 'I':	/* modify away; we don't care */
			case 'D':
			case 'd':
			case 'i':
				break;
	
			default:
				goto Unknown;
			}
		}
	}
}
