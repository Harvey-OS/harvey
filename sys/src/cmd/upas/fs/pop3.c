#include "common.h"
#include <ctype.h>
#include <plumb.h>
#include <libsec.h>
#include <auth.h>
#include "dat.h"

#pragma varargck type "M" uchar*
#pragma varargck argpos pop3cmd 2

typedef struct Pop Pop;
struct Pop {
	char *freep;	// free this to free the strings below

	char *host;
	char *user;

	int ppop;
	int refreshtime;
	int debug;
	int pipeline;
	int hastls;
	int needtls;

	// open network connection
	Biobuf bin;
	Biobuf bout;
	int fd;

	Thumbprint *thumb;
};

char*
geterrstr(void)
{
	static char err[64];

	err[0] = '\0';
	errstr(err, sizeof(err));
	return err;
}

//
// get pop3 response line , without worrying
// about multiline responses; the clients
// will deal with that.
//
static int
isokay(char *s)
{
	return s!=nil && strncmp(s, "+OK", 3)==0;
}

static void
pop3cmd(Pop *pop, char *fmt, ...)
{
	char buf[128], *p;
	va_list va;

	va_start(va, fmt);
	vseprint(buf, buf+sizeof(buf), fmt, va);
	va_end(va);

	p = buf+strlen(buf);
	if(p > (buf+sizeof(buf)-3))
		sysfatal("pop3 command too long");

	if(pop->debug)
		fprint(2, "<- %s\n", buf);
	strcpy(p, "\r\n");
	Bwrite(&pop->bout, buf, strlen(buf));
	Bflush(&pop->bout);
}

static char*
pop3resp(Pop *pop)
{
	char *s;
	char *p;

	if((s = Brdline(&pop->bin, '\n')) == nil)
		return nil;

	p = s+Blinelen(&pop->bin)-1;
	while(p >= s && (*p == '\r' || *p == '\n'))
		*p-- = '\0';

	if(pop->debug)
		fprint(2, "-> %s\n", s);
	return s;
}

//
// get capability list, possibly start tls
//
static char*
pop3capa(Pop *pop)
{
	int fd;
	char *s;
	uchar digest[SHA1dlen];
	int hastls;
	TLSconn conn;

	pop3cmd(pop, "CAPA");
	if(!isokay(pop3resp(pop)))
		return nil;

	hastls = 0;
	while(s = pop3resp(pop)){
		if(strcmp(s, ".") == 0)
			break;
		if(strcmp(s, "STLS") == 0)
			hastls = 1;
		if(strcmp(s, "PIPELINING") == 0)
			pop->pipeline = 1;
	}

	if(hastls){
		pop3cmd(pop, "STLS");
		if(!isokay(s = pop3resp(pop)))
			return s;
		memset(&conn, 0, sizeof conn);
		fd = tlsClient(pop->fd, &conn);
		if(fd < 0)
			sysfatal("tlsClient: %r");
		if(conn.cert==nil || conn.certlen <= 0)
			sysfatal("server did not provide TLS certificate");
		sha1(conn.cert, conn.certlen, digest, nil);
		if(!pop->thumb || !okThumbprint(digest, pop->thumb)){
			fmtinstall('H', encodefmt);
			sysfatal("server certificate %.*H not recognized", SHA1dlen, digest);
		}
		free(conn.cert);
		close(pop->fd);
		pop->fd = fd;
		Binit(&pop->bin, fd, OREAD);
		Binit(&pop->bout, fd, OWRITE);
		pop->hastls = 1;
	}
	return nil;
}

//
// log in using APOP if possible, password if allowed by user
//
static char*
pop3login(Pop *pop)
{
	int n;
	char *s, *p, *q;
	char ubuf[128], user[128];
	char buf[500];
	UserPasswd *up;

	s = pop3resp(pop);
	if(!isokay(s))
		return "error in initial handshake";

	if(pop->user)
		snprint(ubuf, sizeof ubuf, " user=%q", pop->user);
	else
		ubuf[0] = '\0';

	// look for apop banner
	if(pop->ppop==0 && (p = strchr(s, '<')) && (q = strchr(p+1, '>'))) {
		*++q = '\0';
		if((n=auth_respond(p, q-p, user, sizeof user, buf, sizeof buf, auth_getkey, "proto=apop role=client server=%q%s",
			pop->host, ubuf)) < 0)
			return "factotum failed";
		if(user[0]=='\0')
			return "factotum did not return a user name";

		if(s = pop3capa(pop))
			return s;

		pop3cmd(pop, "APOP %s %.*s", user, n, buf);
		if(!isokay(s = pop3resp(pop)))
			return s;

		return nil;
	} else {
		if(pop->ppop == 0)
			return "no APOP hdr from server";

		if(s = pop3capa(pop))
			return s;

		if(pop->needtls && !pop->hastls)
			return "could not negotiate TLS";

		up = auth_getuserpasswd(auth_getkey, "proto=pass service=pop dom=%q%s",
			pop->host, ubuf);
		if(up == nil)
			return "no usable keys found";

		pop3cmd(pop, "USER %s", up->user);
		if(!isokay(s = pop3resp(pop))){
			free(up);
			return s;
		}
		pop3cmd(pop, "PASS %s", up->passwd);
		free(up);
		if(!isokay(s = pop3resp(pop)))
			return s;

		return nil;
	}
}

//
// dial and handshake with pop server
//
static char*
pop3dial(Pop *pop)
{
	char *err;

	if((pop->fd = dial(netmkaddr(pop->host, "net", "pop3"), 0, 0, 0)) < 0)
		return geterrstr();

	Binit(&pop->bin, pop->fd, OREAD);
	Binit(&pop->bout, pop->fd, OWRITE);

	if(err = pop3login(pop)) {
		close(pop->fd);
		return err;
	}

	return nil;
}

//
// close connection
//
static void
pop3hangup(Pop *pop)
{
	pop3cmd(pop, "QUIT");
	pop3resp(pop);
	close(pop->fd);
}

//
// download a single message
//
static char*
pop3download(Pop *pop, Message *m)
{
	char *s, *f[3], *wp, *ep;
	char sdigest[SHA1dlen*2+1];
	int i, l, sz;

	if(!pop->pipeline)
		pop3cmd(pop, "LIST %d", m->mesgno);
	if(!isokay(s = pop3resp(pop)))
		return s;

	if(tokenize(s, f, 3) != 3)
		return "syntax error in LIST response";

	if(atoi(f[1]) != m->mesgno)
		return "out of sync with pop3 server";

	sz = atoi(f[2])+200;	/* 200 because the plan9 pop3 server lies */
	if(sz == 0)
		return "invalid size in LIST response";

	m->start = wp = emalloc(sz+1);
	ep = wp+sz;

	if(!pop->pipeline)
		pop3cmd(pop, "RETR %d", m->mesgno);
	if(!isokay(s = pop3resp(pop))) {
		m->start = nil;
		free(wp);
		return s;
	}

	s = nil;
	while(wp <= ep) {
		s = pop3resp(pop);
		if(s == nil) {
			free(m->start);
			m->start = nil;
			return "unexpected end of conversation";
		}
		if(strcmp(s, ".") == 0)
			break;

		l = strlen(s)+1;
		if(s[0] == '.') {
			s++;
			l--;
		}
		if(wp+l > ep) {
			free(m->start);
			m->start = nil;
			while(s = pop3resp(pop)) {
				if(strcmp(s, ".") == 0)
					break;
			}
			return "message larger than expected";
		}
		memmove(wp, s, l-1);
		wp[l-1] = '\n';
		wp += l;
	}

	if(s == nil || strcmp(s, ".") != 0)
		return "out of sync with pop3 server";

	m->end = wp;

	// make sure there's a trailing null
	// (helps in body searches)
	*m->end = 0;
	m->bend = m->rbend = m->end;
	m->header = m->start;

	// digest message
	sha1((uchar*)m->start, m->end - m->start, m->digest, nil);
	for(i = 0; i < SHA1dlen; i++)
		sprint(sdigest+2*i, "%2.2ux", m->digest[i]);
	m->sdigest = s_copy(sdigest);

	return nil;
}

//
// check for new messages on pop server
// UIDL is not required by RFC 1939, but 
// netscape requires it, so almost every server supports it.
// we'll use it to make our lives easier.
//
static char*
pop3read(Pop *pop, Mailbox *mb, int doplumb)
{
	char *s, *p, *uidl, *f[2];
	int mesgno, ignore, nnew;
	Message *m, *next, **l;

	// Some POP servers disallow UIDL if the maildrop is empty.
	pop3cmd(pop, "STAT");
	if(!isokay(s = pop3resp(pop)))
		return s;

	// fetch message listing; note messages to grab
	l = &mb->root->part;
	if(strncmp(s, "+OK 0 ", 6) != 0) {
		pop3cmd(pop, "UIDL");
		if(!isokay(s = pop3resp(pop)))
			return s;

		while(p = pop3resp(pop)) {
			if(strcmp(p, ".") == 0)
				break;

			if(tokenize(p, f, 2) != 2)
				continue;

			mesgno = atoi(f[0]);
			uidl = f[1];
			if(strlen(uidl) > 75)	// RFC 1939 says 70 characters max
				continue;

			ignore = 0;
			while(*l != nil) {
				if(strcmp((*l)->uidl, uidl) == 0) {
					// matches mail we already have, note mesgno for deletion
					(*l)->mesgno = mesgno;
					ignore = 1;
					l = &(*l)->next;
					break;
				} else {
					// old mail no longer in box mark deleted
					if(doplumb)
						mailplumb(mb, *l, 1);
					(*l)->inmbox = 0;
					(*l)->deleted = 1;
					l = &(*l)->next;
				}
			}
			if(ignore)
				continue;

			m = newmessage(mb->root);
			m->mallocd = 1;
			m->inmbox = 1;
			m->mesgno = mesgno;
			strcpy(m->uidl, uidl);

			// chain in; will fill in message later
			*l = m;
			l = &m->next;
		}
	}

	// whatever is left has been removed from the mbox, mark as deleted
	while(*l != nil) {
		if(doplumb)
			mailplumb(mb, *l, 1);
		(*l)->inmbox = 0;
		(*l)->deleted = 1;
		l = &(*l)->next;
	}

	// download new messages
	nnew = 0;
	if(pop->pipeline){
		switch(rfork(RFPROC|RFMEM)){
		case -1:
			fprint(2, "rfork: %r\n");
			pop->pipeline = 0;

		default:
			break;

		case 0:
			for(m = mb->root->part; m != nil; m = m->next){
				if(m->start != nil)
					continue;
				Bprint(&pop->bout, "LIST %d\r\nRETR %d\r\n", m->mesgno, m->mesgno);
			}
			Bflush(&pop->bout);
			_exits(nil);
		}
	}

	for(m = mb->root->part; m != nil; m = next) {
		next = m->next;

		if(m->start != nil)
			continue;

		if(s = pop3download(pop, m)) {
			// message disappeared? unchain
			fprint(2, "download %d: %s\n", m->mesgno, s);
			delmessage(mb, m);
			mb->root->subname--;
			continue;
		}
		nnew++;
		parse(m, 0, mb);

		if(doplumb)
			mailplumb(mb, m, 0);
	}
	if(pop->pipeline)
		waitpid();

	if(nnew) {
		mb->vers++;
		henter(PATH(0, Qtop), mb->name,
			(Qid){PATH(mb->id, Qmbox), mb->vers, QTDIR}, nil, mb);
	}

	return nil;	
}

//
// delete marked messages
//
static void
pop3purge(Pop *pop, Mailbox *mb)
{
	Message *m, *next;

	for(m = mb->root->part; m != nil; m = next) {
		next = m->next;
		if(m->deleted && m->refs == 0) {
			if(m->inmbox) {
				pop3cmd(pop, "DELE %d", m->mesgno);
				if(isokay(pop3resp(pop)))
					delmessage(mb, m);
			} else
				delmessage(mb, m);
		}
	}
}


// connect to pop3 server, sync mailbox
static char*
pop3sync(Mailbox *mb, int doplumb)
{
	char *err;
	Pop *pop;

	pop = mb->aux;

	if(err = pop3dial(pop)) {
		mb->waketime = time(0) + pop->refreshtime;
		return err;
	}

	if((err = pop3read(pop, mb, doplumb)) == nil){
		pop3purge(pop, mb);
		mb->d->atime = mb->d->mtime = time(0);
	}
	pop3hangup(pop);
	mb->waketime = time(0) + pop->refreshtime;
	return err;
}

static char Epop3ctl[] = "bad pop3 control message";

static char*
pop3ctl(Mailbox *mb, int argc, char **argv)
{
	int n;
	Pop *pop;

	pop = mb->aux;
	if(argc < 1)
		return Epop3ctl;

	if(argc==1 && strcmp(argv[0], "debug")==0){
		pop->debug = 1;
		return nil;
	}

	if(argc==1 && strcmp(argv[0], "nodebug")==0){
		pop->debug = 0;
		return nil;
	}

	if(argc==1 && strcmp(argv[0], "thumbprint")==0){
		if(pop->thumb)
			freeThumbprints(pop->thumb);
		pop->thumb = initThumbprints("/sys/lib/tls/mail", "/sys/lib/tls/mail.exclude");
	}
	if(strcmp(argv[0], "refresh")==0){
		if(argc==1){
			pop->refreshtime = 60;
			return nil;
		}
		if(argc==2){
			n = atoi(argv[1]);
			if(n < 15)
				return Epop3ctl;
			pop->refreshtime = n;
			return nil;
		}
	}

	return Epop3ctl;
}

// free extra memory associated with mb
static void
pop3close(Mailbox *mb)
{
	Pop *pop;

	pop = mb->aux;
	free(pop->freep);
	free(pop);
}

//
// open mailboxes of the form /pop/host/user or /apop/host/user
//
char*
pop3mbox(Mailbox *mb, char *path)
{
	char *f[10];
	int nf, apop, ppop, apoptls, poptls;
	Pop *pop;

	quotefmtinstall();
	poptls = strncmp(path, "/poptls/", 8) == 0;
	ppop = poptls || strncmp(path, "/pop/", 5) == 0;
	apoptls = strncmp(path, "/apoptls/", 9) == 0;
	apop = apoptls || strncmp(path, "/apop/", 6) == 0;
	
	if(!ppop && !apop)
		return Enotme;

	path = strdup(path);
	if(path == nil)
		return "out of memory";

	nf = getfields(path, f, nelem(f), 0, "/");
	if(nf != 3 && nf != 4) {
		free(path);
		return "bad pop3 path syntax /[a]pop[tls]/system[/user]";
	}

	pop = emalloc(sizeof(*pop));
	pop->freep = path;
	pop->host = f[2];
	if(nf < 4)
		pop->user = nil;
	else
		pop->user = f[3];
	pop->ppop = ppop;
	pop->needtls = poptls || apoptls;
	pop->refreshtime = 60;
	pop->thumb = initThumbprints("/sys/lib/tls/mail", "/sys/lib/tls/mail.exclude");

	mb->aux = pop;
	mb->sync = pop3sync;
	mb->close = pop3close;
	mb->ctl = pop3ctl;
	mb->d = emalloc(sizeof(*mb->d));

	return nil;
}

