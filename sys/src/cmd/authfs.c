#include <u.h>
#include <libc.h>
#include <auth.h>
#include <fcall.h>
#include <thread.h>
#include <9p.h>

Tree *authtree;

typedef struct Crypt	Crypt;
struct Crypt
{
	Ticket		t;
	Authenticator	a;
	char		tbuf[TICKETLEN];	/* remote ticket */
};

enum {
	Qauthcheck,
	Qauthenticator,
	Qauthserver,
	Qcs,
	Qhostowner,
	Qkey,
	Quser,
};

char user[NAMELEN];
char hostowner[NAMELEN];
char authserver[NAMELEN];
char key[DESKEYLEN];

static char Ebadarg[] = "bad argument in system call";
static char Eperm[] = "permission denied";

Crypt*
newcrypt(void)
{
	Crypt *c;

	c = malloc(sizeof *c);
	if(c == nil)
		sysfatal("out of memory");
	memset(c, 0, sizeof *c);
	return c;
}

void
freecrypt(Crypt *c)
{
	free(c);
}

char*
oserror(void)
{
	static char buf[ERRLEN];

	buf[0] = 0;
	errstr(buf);
	return buf;
}

void
authopen(Req *r, Fid *fid, int omode, Qid*)
{
	int fd;

	switch((long)fid->file->aux){
	case Qcs:
		fd = open("/net/cs", omode);
		if(fd < 0){
			respond(r, oserror());
			return;
		}

		fid->aux = (void*)fd;
		break;
	}
	respond(r, nil);
}

/*
 *  called by devcons() for #c/authcheck
 *
 *  a write of a ticket+authenticator [+challenge+id] succeeds if they match
 */
char*
authcheckwrite(Fid *c, char *a, long *n, vlong)
{
	Crypt *cp;
	char *chal;
	ulong id;

	if(*n != TICKETLEN+AUTHENTLEN && *n != TICKETLEN+AUTHENTLEN+CHALLEN+4)
		return Ebadarg;
	if(c->aux == 0)
		c->aux = newcrypt();
	cp = c->aux;

	memmove(cp->tbuf, a, TICKETLEN);
	convM2T(cp->tbuf, &cp->t, key);
	if(cp->t.num != AuthTc)
		return(Ebadarg);
	if(strcmp(user, cp->t.cuid))
		return(cp->t.cuid);

	memmove(cp->tbuf, a+TICKETLEN, AUTHENTLEN);
	convM2A(cp->tbuf, &cp->a, cp->t.key);
	if(*n == TICKETLEN+AUTHENTLEN+CHALLEN+4){
		uchar *p = (uchar *)&a[TICKETLEN+AUTHENTLEN+CHALLEN];
		id = p[0] | (p[1]<<8) | (p[2]<<16) | (p[3]<<24);
		chal = &a[TICKETLEN+AUTHENTLEN];
	}else{
		id = 0;
		chal = cp->t.chal;
	}
	if(cp->a.num != AuthAs || memcmp(chal, cp->a.chal, CHALLEN) || cp->a.id != id)
		return(Eperm);
	return nil;
}

/*
 *  reading authcheck after writing into it yields the
 *  unencrypted ticket
 */
char*
authcheckread(Fid *c, char *a, long *n, vlong)
{
	Crypt *cp;

	cp = c->aux;
	if(cp == nil)
		return(Ebadarg);
	if(*n < TICKETLEN)
		return(Ebadarg);
	convT2M(&cp->t, a, nil);
	*n = sizeof(cp->t);
	return nil;
}

/*
 *  called by devcons() for #c/authenticator
 *
 *  a read after a write of a ticket (or ticket+id) returns an authenticator
 *  for that ticket.
 */
char*
authentwrite(Fid *c, char *a, long *n, vlong)
{
	Crypt *cp;

	if(*n != TICKETLEN && *n != TICKETLEN+4)
		return(Ebadarg);
	if(c->aux == 0)
		c->aux = newcrypt();
	cp = c->aux;

	memmove(cp->tbuf, a, TICKETLEN);
	convM2T(cp->tbuf, &cp->t, key);
	if(cp->t.num != AuthTc || strcmp(cp->t.cuid, user)){
		freecrypt(cp);
		c->aux = 0;
		return(Ebadarg);
	}
	if(*n == TICKETLEN+4){
		uchar *p = (uchar *)&a[TICKETLEN];
		cp->a.id = p[0] | (p[1]<<8) | (p[2]<<16) | (p[3]<<24);
	}else
		cp->a.id = 0;

	return nil;
}

/*
 *  create an authenticator and return it and optionally the
 *  unencripted ticket
 */
char*
authentread(Fid *c, char *a, long *n, vlong)
{
	Crypt *cp;

	cp = c->aux;
	if(cp == 0)
		return("authenticator read must follow a write");

	cp->a.num = AuthAc;
	memmove(cp->a.chal, cp->t.chal, CHALLEN);
	convA2M(&cp->a, cp->tbuf, cp->t.key);

	if(*n >= AUTHENTLEN)
		memmove(a, cp->tbuf, AUTHENTLEN);
	return nil;
}

char*
keyread(Fid*, char*, long *n, vlong offset)
{
	if(*n<DESKEYLEN || offset != 0)
		return(Ebadarg);
	return(Eperm);
}

char*
keywrite(Fid*, char *a, long *n, vlong)
{
	if(*n != DESKEYLEN)
		return(Ebadarg);
	memmove(key, a, DESKEYLEN);
	return nil;
}

void
authclunkaux(Fid *fid)
{
	switch((long)fid->file->aux){
	case Qauthcheck:
	case Qauthenticator:
		free(fid->aux);
		fid->aux = nil;
		break;
	case Qcs:
		close((int)fid->aux);
		fid->aux = (void*)-1;
		break;
	}
}

void
authread(Req *r, Fid *fid, void *buf, long *count, vlong offset)
{
	int n;
	switch((long)fid->file->aux){
	case Qauthcheck:
		respond(r, authcheckread(fid, buf, count, offset));
		break;
	case Qauthenticator:
		respond(r, authentread(fid, buf, count, offset));
		break;
	case Qauthserver:
		readstr(offset, buf, count, authserver);
		respond(r, nil);
		break;
	case Qcs:
		if(offset != 0){
			*count = 0;
			respond(r, nil);
			break;
		}
		seek((int)fid->aux, 0, 0);
		n = read((int)fid->aux, buf, *count);
		if(n < 0)
			respond(r, oserror());
		else{
			*count = n;
			respond(r, nil);
		}
		break;
	case Qhostowner:
		readstr(offset, buf, count, hostowner);
		respond(r, nil);
		break;
	case Qkey:
		respond(r, keyread(fid, buf, count, offset));
		break;
	case Quser:
		readstr(offset, buf, count, user);
		respond(r, nil);
		break;
	default:
		abort();
	}
}

char Eoffset[] = "nonzero offset in write";

char*
namewrite(char *dst, char *src, long *count, vlong offset)
{
	int i;
	if(offset != 0)
		return Eoffset;
	if(*count < 0 || *count > NAMELEN)
		return "bad count in write";

	memmove(dst, src, *count);
	if(*count >= NAMELEN)
		i = NAMELEN-1;
	else
		i = *count;
	dst[i] = 0;
	while(i > 0 && dst[i-1] == ' ' || dst[i-1] == '\n' || dst[i-1] == '\t')
		dst[--i] = 0;
	
	return nil;
}

void
authwrite(Req *r, Fid *fid, void *buf, long *count, vlong offset)
{
	char s[128];
	char t[128];
	char *p;

	switch((long)fid->file->aux){
	case Qauthcheck:
		respond(r, authcheckwrite(fid, buf, count, offset));
		break;
	case Qauthenticator:
		respond(r, authentwrite(fid, buf, count, offset));
		break;
	case Qauthserver:
		respond(r, namewrite(authserver, buf, count, offset));
		break;
	case Qcs:
		if(*count > sizeof(s)-1){
			respond(r, "count too big");
			break;
		}
		memmove(s, buf, *count);
		s[*count] = 0;
		if(p = strstr(s, "$auth"))
			snprint(t, sizeof t, "%.*s%s%s", utfnlen(s, p-s), s, authserver, p+strlen("$auth"));
		else
			strcpy(t, s);

		seek((int)fid->aux, 0, 0);
		if(write((int)fid->aux, t, strlen(t)) != strlen(t)){
			respond(r, oserror());
			break;
		}
		respond(r, nil);
		break;
	case Qhostowner:
		respond(r, namewrite(hostowner, buf, count, offset));
		break;
	case Qkey:
		respond(r, keywrite(fid, buf, count, offset));
		break;
	case Quser:
		respond(r, namewrite(user, buf, count, offset));
		break;
	default:
		abort();
	}
}

void
mkfile(char *name, int v)
{
	File *f;
	f = fcreate(authtree->root, name, user, user, 0666);
	if(f == nil)
		sysfatal("creating %s", name);
	f->aux = (void*)v;
}

Srv authsrv = {
	.open=	authopen,
	.read=	authread,
	.write=	authwrite,
	.clunkaux=	authclunkaux,
};

int
readln(char *prompt, char *line, int len)
{
	char *p;
	int fd, ctl, n, nr;

	fd = open("/dev/cons", ORDWR);
	if(fd < 0)
		sysfatal("couldn't open cons");
	ctl = open("/dev/consctl", OWRITE);
	if(ctl < 0)
		sysfatal("couldn't set raw mode");
	write(ctl, "rawon", 5);
	fprint(fd, "%s", prompt);
	nr = 0;
	p = line;
	for(;;){
		n = read(fd, p, 1);
		if(n < 0){
			close(fd);
			close(ctl);
			return -1;
		}
		if(n == 0 || *p == '\n' || *p == '\r'){
			*p = '\0';
			write(fd, "\n", 1);
			close(fd);
			close(ctl);
			return nr;
		}
		if(*p == '\b'){
			if(nr > 0){
				nr--;
				p--;
			}
		}else if(*p == 21){		/* cntrl-u */
			fprint(fd, "\n%s", prompt);
			nr = 0;
			p = line;
		}else{
			nr++;
			p++;
		}
		if(nr == len){
			fprint(fd, "line too long; try again\n%s", prompt);
			nr = 0;
			p = line;
		}
	}
	return -1;
}

void
usage(void)
{
	fprint(2, "usage: authfs [-a authsrv] [-u user] [-h hostowner] [-p]\n");
	exits("usage");
}

void
main(int argc, char **argv)
{
	char *u;
	long l;
	char pass[32];

	rfork(RFNOTEG);
	strcpy(authserver, "$auth");

	u = getuser();
	if(u == nil)
		u = "none";
	strcpy(user, u);

	ARGBEGIN{
	case 'a':
		u = EARGF(usage());
		l = strlen(u);
		namewrite(authserver, u, &l, 0);
		break;
	case 'u':
		u = EARGF(usage());
		l = strlen(u);
		namewrite(user, u, &l, 0);
		break;
	case 'h':
		u = EARGF(usage());
		l = strlen(u);
		namewrite(hostowner, u, &l, 0);
		break;
	case 'p':
		readln("Password: ", pass, sizeof pass);
		passtokey(key, pass);
		break;
	case 'v':
		lib9p_chatty++;
		break;
	default:
		usage();
	}ARGEND;

	if(hostowner[0] == 0)
		strcpy(hostowner, user);

	if(argc != 0)
		usage();

	authtree = mktree(user, user, CHDIR|0555);
	authsrv.tree = authtree;
	mkfile("authcheck", Qauthcheck);
	mkfile("authenticator", Qauthenticator);
	mkfile("authserver", Qauthserver);
	mkfile("cs", Qcs);
	mkfile("hostowner", Qhostowner);
	mkfile("key", Qkey);
	mkfile("user", Quser);

	postmountsrv(&authsrv, nil, "/dev", MBEFORE);
	bind("/dev/cs", "/net/cs", MREPL);
	exits(0);
}
