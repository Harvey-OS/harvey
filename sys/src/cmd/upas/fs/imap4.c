#include "common.h"
#include <ctype.h>
#include <plumb.h>
#include <libsec.h>
#include <auth.h>
#include "dat.h"

#pragma varargck argpos imap4cmd 2

static char Eio[] = "i/o error";

typedef struct Imap Imap;
struct Imap {
	char *freep;	// free this to free the strings below

	char *host;
	char *user;
	char *mbox;

	int mustssl;
	int refreshtime;
	int debug;

	ulong tag;
	ulong validity;
	int nmsg;
	int size;
	char *base;
	char *data;

	vlong *uid;
	int nuid;
	int muid;

	Thumbprint *thumb;

	// open network connection
	Biobuf bin;
	Biobuf bout;
	int fd;
};

static char*
removecr(char *s)
{
	char *r, *w;

	for(r=w=s; *r; r++)
		if(*r != '\r')
			*w++ = *r;
	*w = '\0';
	return s;
}

//
// send imap4 command
//
static void
imap4cmd(Imap *imap, char *fmt, ...)
{
	char buf[128], *p;
	va_list va;

	va_start(va, fmt);
	p = buf+sprint(buf, "9X%lud ", imap->tag);
	vseprint(p, buf+sizeof(buf), fmt, va);
	va_end(va);

	p = buf+strlen(buf);
	if(p > (buf+sizeof(buf)-3))
		sysfatal("imap4 command too long");

	if(imap->debug)
		fprint(2, "-> %s\n", buf);
	strcpy(p, "\r\n");
	Bwrite(&imap->bout, buf, strlen(buf));
	Bflush(&imap->bout);
}

enum {
	OK,
	NO,
	BAD,
	BYE,
	EXISTS,
	STATUS,
	FETCH,
	UNKNOWN,
};

static char *verblist[] = {
[OK]		"OK",
[NO]		"NO",
[BAD]	"BAD",
[BYE]	"BYE",
[EXISTS]	"EXISTS",
[STATUS]	"STATUS",
[FETCH]	"FETCH",
};

static int
verbcode(char *verb)
{
	int i;
	char *q;

	if(q = strchr(verb, ' '))
		*q = '\0';

	for(i=0; i<nelem(verblist); i++)
		if(verblist[i] && strcmp(verblist[i], verb)==0){
			if(q)
				*q = ' ';
			return i;
		}
	if(q)
		*q = ' ';
	return UNKNOWN;
}

static void
strupr(char *s)
{
	for(; *s; s++)
		if('a' <= *s && *s <= 'z')
			*s += 'A'-'a';
}


//
// get imap4 response line.  there might be various 
// data or other informational lines mixed in.
//
static char*
imap4resp(Imap *imap)
{
	char *line, *p, *ep, *op, *q, *r, *en, *verb;
	int n;

	while(p = Brdline(&imap->bin, '\n')){
		ep = p+Blinelen(&imap->bin);
		while(ep > p && (ep[-1]=='\n' || ep[-1]=='\r'))
			*--ep = '\0';
		
		if(imap->debug)
			fprint(2, "<- %s\n", p);
		strupr(p);

		switch(p[0]){
		case '+':
			if(imap->tag == 0)
				fprint(2, "unexpected: %s\n", p);
			break;

		// ``unsolicited'' information; everything happens here.
		case '*':
			if(p[1]!=' ')
				continue;
			p += 2;
			line = p;
			n = strtol(p, &p, 10);
			if(*p==' ')
				p++;
			verb = p;
			
			if(p = strchr(verb, ' '))
				p++;
			else
				p = verb+strlen(verb);

			switch(verbcode(verb)){
			case OK:
			case NO:
			case BAD:
				// human readable text at p;
				break;
			case BYE:
				// early disconnect
				// human readable text at p;
				break;

			// * 32 EXISTS
			case EXISTS:
				imap->nmsg = n;
				break;

			// * STATUS Inbox (MESSAGES 2 UIDVALIDITY 960164964)
			case STATUS:
				if(q = strstr(p, "MESSAGES"))
					imap->nmsg = atoi(q+8);
				if(q = strstr(p, "UIDVALIDITY"))
					imap->validity = strtoul(q+11, 0, 10);
				break;

			case FETCH:
				// * 1 FETCH (UID 1 RFC822.SIZE 511)
				if(q=strstr(p, "RFC822.SIZE")){
					imap->size = atoi(q+11);
					break;
				}

				// * 1 FETCH (UID 1 RFC822.HEADER {496}
				// <496 bytes of data>
 				// )
				// * 1 FETCH (UID 1 RFC822.HEADER "data")
				if(strstr(p, "RFC822.HEADER") || strstr(p, "RFC822.TEXT")){
					if((q = strchr(p, '{')) 
					&& (n=strtol(q+1, &en, 0), *en=='}')){
						if(imap->data == nil){
							imap->base = emalloc(n+1);	
							imap->data = imap->base;
							imap->size = n+1;
						}
						if(n >= imap->size)
							return Eio;
						if(Bread(&imap->bin, imap->data, n) != n)
							return Eio;
						imap->data[n] = '\0';
						imap->data += n;
						imap->size -= n;
						Brdline(&imap->bin, '\n');
					}else if((q = strchr(p, '"')) && (r = strchr(q+1, '"'))){
						*r = '\0';
						q++;
						n = r-q;
						if(imap->data == nil){
							imap->base = emalloc(n+1);
							imap->data = imap->base;
							imap->size = n+1;
						}
						if(n >= imap->size)
							return Eio;
						memmove(imap->data, q, n);
						imap->data[n] = '\0';
						imap->data += n;
						imap->size -= n;
					}else
						return "confused about FETCH response";
					break;
				}

				// * 1 FETCH (UID 1)
				// * 2 FETCH (UID 6)
				if(q = strstr(p, "UID")){
					if(imap->nuid < imap->muid)
						imap->uid[imap->nuid++] = ((vlong)imap->validity<<32)|strtoul(q+3, nil, 10);
					break;
				}
			}

			if(imap->tag == 0)
				return line;
			break;

		case '9':		// response to our message
			op = p;
			if(p[1]=='X' && strtoul(p+2, &p, 10)==imap->tag){
				while(*p==' ')
					p++;
				imap->tag++;
				return p;
			}
			fprint(2, "expected %lud; got %s\n", imap->tag, op);
			break;

		default:
			fprint(2, "unexpected line: %s\n", p);
		}
	}
	return Eio;
}

static int
isokay(char *resp)
{
	return strncmp(resp, "OK", 2)==0;
}

//
// log in to IMAP4 server, select mailbox, no SSL at the moment
//
static char*
imap4login(Imap *imap)
{
	char *s;
	UserPasswd *up;

	imap->tag = 0;
	s = imap4resp(imap);
	if(!isokay(s) || strstr(s, "IMAP4")==0)
		return "error in initial handshake";

	if(imap->user != nil)
		up = auth_getuserpasswd(auth_getkey, "proto=pass service=imap server=%q user=%q", imap->host, imap->user);
	else
		up = auth_getuserpasswd(auth_getkey, "proto=pass service=imap server=%q", imap->host);
	if(up == nil)
		return "cannot find password";

	imap->tag = 1;
	imap4cmd(imap, "LOGIN %s %s", up->user, up->passwd);
	free(up);
	if(!isokay(s = imap4resp(imap)))
		return s;

	imap4cmd(imap, "SELECT %s", imap->mbox);
	if(!isokay(s = imap4resp(imap)))
		return s;

	return nil;
}

//
// push tls onto a connection
//
int
mypushtls(int fd)
{
	int p[2];
	char buf[10];

	if(pipe(p) < 0)
		return -1;

	switch(fork()){
	case -1:
		close(p[0]);
		close(p[1]);
		return -1;
	case 0:
		close(p[1]);
		dup(p[0], 0);
		dup(p[0], 1);
		sprint(buf, "/fd/%d", fd);
		execl("/bin/tlsrelay", "tlsrelay", "-f", buf, nil);
		_exits(nil);
	default:
		break;
	}
	close(fd);
	close(p[0]);
	return p[1];
}

//
// dial and handshake with the imap server
//
static char*
imap4dial(Imap *imap)
{
	char *err, *port;
	uchar digest[SHA1dlen];
	int sfd;
	TLSconn conn;

	if(imap->fd >= 0){
		imap4cmd(imap, "noop");
		if(isokay(imap4resp(imap)))
			return nil;
		close(imap->fd);
		imap->fd = -1;
	}

	if(imap->mustssl)
		port = "imaps";
	else
		port = "imap4";

	if((imap->fd = dial(netmkaddr(imap->host, "net", port), 0, 0, 0)) < 0)
		return geterrstr();

	if(imap->mustssl){
		memset(&conn, 0, sizeof conn);
		sfd = tlsClient(imap->fd, &conn);
		if(sfd < 0)
			sysfatal("tlsClient: %r");
		if(conn.cert==nil || conn.certlen <= 0)
			sysfatal("server did not provide TLS certificate");
		sha1(conn.cert, conn.certlen, digest, nil);
		if(!imap->thumb || !okThumbprint(digest, imap->thumb)){
			fmtinstall('H', encodefmt);
			sysfatal("server certificate %.*H not recognized", SHA1dlen, digest);
		}
		free(conn.cert);
		close(imap->fd);
		imap->fd = sfd;
	}
	Binit(&imap->bin, imap->fd, OREAD);
	Binit(&imap->bout, imap->fd, OWRITE);

	if(err = imap4login(imap)) {
		close(imap->fd);
		return err;
	}

	return nil;
}

//
// close connection
//
static void
imap4hangup(Imap *imap)
{
	imap4cmd(imap, "LOGOUT");
	imap4resp(imap);
	close(imap->fd);
}

//
// download just the header of a message
// because we do this, the sha1 digests are only of
// the headers.  hopefully this won't cause problems.
//
// this doesn't work (and isn't used).  the downloading
// itself works, but we need to have the full mime headers
// to get the right recursive structures into the file system,
// which means having the body.  i've also had other weird
// problems with bodies being paged in incorrectly.
//
// this is here as a start in case someone else wants a crack
// at it.  note that if you start using this you'll have to 
// change the digest in imap4fetch() to digest just the header.
//
static char*
imap4fetchheader(Imap *imap, Mailbox *mb, Message *m)
{
	int i;
	char *p, *s, sdigest[2*SHA1dlen+1];

	imap->size = 0;
	free(imap->base);
	imap->base = nil;
	imap->data = nil;

	imap4cmd(imap, "UID FETCH %lud RFC822.HEADER", (ulong)m->imapuid);
	if(!isokay(s = imap4resp(imap)))
		return s;
	if(imap->base == nil)
		return "didn't get header";

	p = imap->base;
	removecr(p);
	free(m->start);
	m->start = p;
	m->end = p+strlen(p);
	m->bend = m->rbend = m->end;
	m->header = m->start;
	//m->fetched = 0;

	imap->base = nil;
	imap->data = nil;

	parseheaders(m, 0, mb);

	// digest headers
	sha1((uchar*)m->start, m->hend - m->start, m->digest, nil);
	for(i = 0; i < SHA1dlen; i++)
		sprint(sdigest+2*i, "%2.2ux", m->digest[i]);
	m->sdigest = s_copy(sdigest);

	return nil;
}

//
// download a single message
//
static char*
imap4fetch(Mailbox *mb, Message *m)
{
	int i, sz;
	char *p, *s, sdigest[2*SHA1dlen+1];
	Imap *imap;

	imap = mb->aux;

//	if(s = imap4dial(imap))
//		return s;
//
//	imap4cmd(imap, "STATUS %s (MESSAGES UIDVALIDITY)", imap->mbox);
//	if(!isokay(s = imap4resp(imap)))
//		return s;
//	if((ulong)(m->imapuid>>32) != imap->validity)
//		return "uids changed underfoot";

	imap->size = 0;
	/* SIZE */
	if(!isokay(s = imap4resp(imap)))
		return s;
	if(imap->size == 0)
		return "didn't get size from size command";

	sz = imap->size+200;	/* 200: slop */
	p = emalloc(sz+1);
	free(imap->base);
	imap->base = p;
	imap->data = p;
	imap->size = sz;

	/* HEADER */
	if(!isokay(s = imap4resp(imap)))
		return s;
	if(imap->size == sz){
		free(p);
		imap->data = nil;
		return "didn't get header";
	}

	/* TEXT */
	if(!isokay(s = imap4resp(imap)))
		return s;

	removecr(p);
	free(m->start);
	m->start = p;
	m->end = p+strlen(p);
	m->bend = m->rbend = m->end;
	m->header = m->start;
	//m->fetched = 1;

	imap->base = nil;
	imap->data = nil;

	parse(m, 0, mb);

	// digest headers
	sha1((uchar*)m->start, m->end - m->start, m->digest, nil);
	for(i = 0; i < SHA1dlen; i++)
		sprint(sdigest+2*i, "%2.2ux", m->digest[i]);
	m->sdigest = s_copy(sdigest);

	return nil;
}

//
// check for new messages on imap4 server
// download new messages, mark deleted messages
//
static char*
imap4read(Imap *imap, Mailbox *mb, int doplumb)
{
	char *s;
	int i, ignore, nnew, t;
	Message *m, *next, **l;

	imap4cmd(imap, "STATUS %s (MESSAGES UIDVALIDITY)", imap->mbox);
	if(!isokay(s = imap4resp(imap)))
		return s;

	imap->nuid = 0;
	imap->uid = erealloc(imap->uid, imap->nmsg*sizeof(imap->uid[0]));
	imap->muid = imap->nmsg;

	imap4cmd(imap, "UID FETCH 1:* UID");
	if(!isokay(s = imap4resp(imap)))
		return s;

	l = &mb->root->part;
	for(i=0; i<imap->nuid; i++){
		ignore = 0;
		while(*l != nil){
			if((*l)->imapuid == imap->uid[i]){
				ignore = 1;
				l = &(*l)->next;
				break;
			}else{
				// old mail, we don't have it anymore
				if(doplumb)
					mailplumb(mb, *l, 1);
				(*l)->inmbox = 0;
				(*l)->deleted = 1;
				l = &(*l)->next;
			}
		}
		if(ignore)
			continue;

		// new message
		m = newmessage(mb->root);
		m->mallocd = 1;
		m->inmbox = 1;
		m->imapuid = imap->uid[i];

		// add to chain, will download soon
		*l = m;
		l = &m->next;
	}

	// whatever is left at the end of the chain is gone
	while(*l != nil){
		if(doplumb)
			mailplumb(mb, *l, 1);
		(*l)->inmbox = 0;
		(*l)->deleted = 1;
		l = &(*l)->next;
	}

	// download new messages
	t = imap->tag;
	switch(rfork(RFPROC|RFMEM)){
	case -1:
		sysfatal("rfork: %r");
	default:
		break;
	case 0:
		for(m = mb->root->part; m != nil; m = m->next){
			if(m->start != nil)
				continue;
			Bprint(&imap->bout, "9X%d UID FETCH %lud RFC822.SIZE\r\n", t++, (ulong)m->imapuid);
			Bprint(&imap->bout, "9X%d UID FETCH %lud RFC822.HEADER\r\n", t++, (ulong)m->imapuid);
			Bprint(&imap->bout, "9X%d UID FETCH %lud RFC822.TEXT\r\n", t++, (ulong)m->imapuid);
		}
		Bflush(&imap->bout);
		_exits(nil);
	}

	nnew = 0;
	for(m=mb->root->part; m!=nil; m=next){
		next = m->next;
		if(m->start != nil)
			continue;

		if(s = imap4fetch(mb, m)){
			// message disappeared?  unchain
			fprint(2, "download %lud: %s\n", (ulong)m->imapuid, s);
			delmessage(mb, m);
			mb->root->subname--;
			continue;
		}
		nnew++;
		if(doplumb)
			mailplumb(mb, m, 0);
	}
	waitpid();

	if(nnew){
		mb->vers++;
		henter(PATH(0, Qtop), mb->name,
			(Qid){PATH(mb->id, Qmbox), mb->vers, QTDIR}, nil, mb);
	}
	return nil;
}

//
// sync mailbox
//
static void
imap4purge(Imap *imap, Mailbox *mb)
{
	int ndel;
	Message *m, *next;

	ndel = 0;
	for(m=mb->root->part; m!=nil; m=next){
		next = m->next;
		if(m->deleted && m->refs==0){
			if(m->inmbox && (ulong)(m->imapuid>>32)==imap->validity){
				imap4cmd(imap, "UID STORE %lud +FLAGS (\\Deleted)", (ulong)m->imapuid);
				if(isokay(imap4resp(imap))){
					ndel++;
					delmessage(mb, m);
				}
			}else
				delmessage(mb, m);
		}
	}

	if(ndel){
		imap4cmd(imap, "EXPUNGE");
		imap4resp(imap);
	}
}

//
// connect to imap4 server, sync mailbox
//
static char*
imap4sync(Mailbox *mb, int doplumb)
{
	char *err;
	Imap *imap;

	imap = mb->aux;

	if(err = imap4dial(imap)){
		mb->waketime = time(0) + imap->refreshtime;
		return err;
	}

	if((err = imap4read(imap, mb, doplumb)) == nil){
		imap4purge(imap, mb);
		mb->d->atime = mb->d->mtime = time(0);
	}
	/*
	 * don't hang up; leave connection open for next time.
	 */
	// imap4hangup(imap);
	mb->waketime = time(0) + imap->refreshtime;
	return err;
}

static char Eimap4ctl[] = "bad imap4 control message";

static char*
imap4ctl(Mailbox *mb, int argc, char **argv)
{
	int n;
	Imap *imap;

	imap = mb->aux;
	if(argc < 1)
		return Eimap4ctl;

	if(argc==1 && strcmp(argv[0], "debug")==0){
		imap->debug = 1;
		return nil;
	}

	if(argc==1 && strcmp(argv[0], "nodebug")==0){
		imap->debug = 0;
		return nil;
	}

	if(argc==1 && strcmp(argv[0], "thumbprint")==0){
		if(imap->thumb)
			freeThumbprints(imap->thumb);
		imap->thumb = initThumbprints("/sys/lib/tls/mail", "/sys/lib/tls/mail.exclude");
	}
	if(strcmp(argv[0], "refresh")==0){
		if(argc==1){
			imap->refreshtime = 60;
			return nil;
		}
		if(argc==2){
			n = atoi(argv[1]);
			if(n < 15)
				return Eimap4ctl;
			imap->refreshtime = n;
			return nil;
		}
	}

	return Eimap4ctl;
}

//
// free extra memory associated with mb
//
static void
imap4close(Mailbox *mb)
{
	Imap *imap;

	imap = mb->aux;
	free(imap->freep);
	free(imap->base);
	free(imap->uid);
	free(imap);
}

//
// open mailboxes of the form /imap/host/user
//
char*
imap4mbox(Mailbox *mb, char *path)
{
	char *f[10];
	int mustssl, nf;
	Imap *imap;

	quotefmtinstall();
	if(strncmp(path, "/imap/", 6) != 0 && strncmp(path, "/imaps/", 7) != 0)
		return Enotme;
	mustssl = (strncmp(path, "/imaps/", 7) == 0);

	path = strdup(path);
	if(path == nil)
		return "out of memory";

	nf = getfields(path, f, 5, 0, "/");
	if(nf < 3){
		free(path);
		return "bad imap path syntax /imap[s]/system[/user[/mailbox]]";
	}

	imap = emalloc(sizeof(*imap));
	imap->fd = -1;
	imap->debug = 0;
	imap->freep = path;
	imap->mustssl = mustssl;
	imap->host = f[2];
	if(nf < 4)
		imap->user = nil;
	else
		imap->user = f[3];
	if(nf < 5)
		imap->mbox = "Inbox";
	else
		imap->mbox = f[4];
	imap->thumb = initThumbprints("/sys/lib/tls/mail", "/sys/lib/tls/mail.exclude");

	mb->aux = imap;
	mb->sync = imap4sync;
	mb->close = imap4close;
	mb->ctl = imap4ctl;
	mb->d = emalloc(sizeof(*mb->d));
	//mb->fetch = imap4fetch;

	return nil;
}
