/*
 *	print mail
 */
#include "common.h"
#include "print.h"
#include "pop3.h"
#include <ctype.h>
#include <libsec.h>

Biobuf in;
Biobuf out;
int alarmed;
int debug;
int fflg;
int interrupted;
int writeable = 1;
int reverse = 1;	/* opposite what edmail does */
char user[NAMELEN];
int loggedin;
String *mailfile;
int passwordinclear;

typedef struct Command Command;
struct Command
{
	char	*name;
	char	needauth;
	int	(*f)(char*);
};

static int deletecmd(char*);
static int listcmd(char*);
static int noopcmd(char*);
static int quitcmd(char*);
static int resetcmd(char*);
static int retrievecmd(char*);
static int statcmd(char*);
static int topcmd(char*);
static int uidlcmd(char*);

Command command[] =
{
	"apop", 0, apopcmd,
	"dele", 1, deletecmd,
	"list", 1, listcmd,
	"noop", 0, noopcmd,
	"pass", 0, passcmd,
	"quit", 0, quitcmd,
	"rset", 0, resetcmd,
	"retr", 1, retrievecmd,
	"stat", 1, statcmd,
	"top", 1, topcmd,
	"uidl", 1, uidlcmd,
	"user", 0, usercmd,
	0, 0, 0,
};

void
main(int argc, char *argv[])
{
	int fd;
	char cmdbuf[64];
	char *arg;
	Command *c;

	Binit(&in, 0, OREAD);
	Binit(&out, 1, OWRITE);

	ARGBEGIN{
	case 'd':
		debug++;
		fd = create("/tmp/pop3", OWRITE, 0666);
		if(fd){
			dup(fd, 2);
			close(fd);
		}
		break;
	case 'p':
		passwordinclear = 1;
		break;
	}ARGEND;

	hello();
	Bflush(&out);

	while(getcrnl(cmdbuf, sizeof(cmdbuf)) > 0){
		Bflush(&out);
		arg = nextarg(cmdbuf);
		for(c = command; c->name; c++)
			if(cistrcmp(c->name, cmdbuf) == 0)
				break;
		if(c->name == 0){
			senderr("unknown command %s", cmdbuf);
			continue;
		}
		if(c->needauth && !loggedin){
			senderr("%s command requires authentication", cmdbuf);
			continue;
		}
		(*c->f)(arg);
	}
	V();
	exits(0);
}

/*
 *  get a line that ends in crnl or cr, turn terminating crnl into a nl
 *
 *  return 0 on EOF
 */
extern int
getcrnl(char *buf, int n)
{
	int c;
	char *ep;
	char *bp;
	Biobuf *fp = &in;

	Bflush(&out);

	bp = buf;
	ep = bp + n - 1;
	while(bp != ep){
		c = Bgetc(fp);
		if(debug) {
			seek(2, 0, 2);
			fprint(2, "%c", c);
		}
		switch(c){
		case -1:
			*bp = 0;
			if(bp==buf)
				return 0;
			else
				return bp-buf;
		case '\r':
			c = Bgetc(fp);
			if(c == '\n'){
				if(debug) {
					seek(2, 0, 2);
					fprint(2, "%c", c);
				}
				*bp = 0;
				return bp-buf;
			}
			Bungetc(fp);
			c = '\r';
			break;
		case '\n':
			*bp = 0;
			return bp-buf;
		}
		*bp++ = c;
	}
	*bp = 0;
	return bp-buf;
}

extern int
sendcrnl(char *fmt, ...)
{
	char buf[1024];
	va_list arg;

	va_start(arg, fmt);
	doprint(buf, buf+sizeof(buf), fmt, arg);
	va_end(arg);
	if(debug)
		fprint(2, "-> %s\n", buf);
	return Bprint(&out, "%s\r\n", buf);
}

extern int
senderr(char *fmt, ...)
{
	char buf[1024];
	va_list arg;

	va_start(arg, fmt);
	doprint(buf, buf+sizeof(buf), fmt, arg);
	va_end(arg);
	if(debug)
		fprint(2, "-> -ERR %s\n", buf);
	return Bprint(&out, "-ERR %s\r\n", buf);
}

extern int
sendok(char *fmt, ...)
{
	char buf[1024];
	va_list arg;

	va_start(arg, fmt);
	doprint(buf, buf+sizeof(buf), fmt, arg);
	va_end(arg);
	if(*buf){
		if(debug)
			fprint(2, "-> +OK %s\n", buf);
		return Bprint(&out, "+OK %s\r\n", buf);
	} else {
		if(debug)
			fprint(2, "-> +OK\n");
		return Bprint(&out, "+OK\r\n");
	}
}

static int
quitcmd(char *arg)
{
	int rv;

	USED(arg);

	rv = 0;
	if(loggedin){
		rv = write_mbox(s_to_c(mailfile), reverse);
		V();
	}
	if(rv != 0)
		senderr("error writing mailbox");
	else
		sendok("");
	exits(0);
	return 0;
}

static int
statcmd(char *arg)
{
	int n, cnt;
	message *m;

	USED(arg);

	n = cnt = 0;
	for(m = mlist; m; m = m->next){
		if(m->status & DELETED)
			continue;
		n++;
		cnt += m->size;
	}
	return sendok("%d %d", n, cnt);
}

static message*
pointtomsg(int n)
{
	message *m;

	if(n < 1)
		return 0;
	n--;

	for(m = mlist; m && n > 0; m = m->next)
		n--;
	return m;
}

static int
listcmd(char *arg)
{
	int n, cnt;
	message *m;

	if(*arg == 0){
		n = cnt = 0;
		for(m = mlist; m; m = m->next){
			if(m->status & DELETED)
				continue;
			n++;
			cnt += m->size;
		}
		sendok("+%d message (%d octets)", n, cnt);
		n = 1;
		for(m = mlist; m; m = m->next){
			if((m->status & DELETED) == 0)
				sendcrnl("%d %d", n, m->size);
			n++;
		}
		sendcrnl(".", n, cnt);
	} else {
		n = atoi(arg);
		m = pointtomsg(n);
		if(m == 0 || (m->status & DELETED))
			return senderr("no such message");
		sendok("%d %d", n, m->size);
	}
	return 0;
}

static int
retrievecmd(char *arg)
{
	message *m;

	if(*arg == 0)
		return senderr("RETR requires a message number");
	m = pointtomsg(atoi(arg));
	if(m == 0 || (m->status & DELETED))
		return senderr("no such message");

	sendok("message follows");
	m_printcrnl(m, &out, 1<<30);
	sendcrnl(".");
	return 0;
}

static int
deletecmd(char *arg)
{
	int n;
	message *m;

	if(*arg == 0)
		return senderr("DELE requires a message number");
	n = atoi(arg);
	m = pointtomsg(n);
	if(m == 0 || (m->status & DELETED))
		return senderr("no such message");
	m->status |= DELETED;
	sendok("message %d deleted", n);
	return 0;
}

static int
noopcmd(char *arg)
{
	USED(arg);
	sendok("");
	return 0;
}

static int
resetcmd(char *arg)
{
	message *m;

	USED(arg);
	for(m = mlist; m; m = m->next)
		m->status &= ~DELETED;
	sendok("");
	return 0;
}

static int
topcmd(char *arg)
{
	message *m;
	char *lines;
	int cnt;

	if(*arg == 0)
		return senderr("TOP requires a message number");
	lines = nextarg(arg);
	if(*lines == 0)
		return senderr("TOP requires a line count");
	m = pointtomsg(atoi(arg));
	if(m == 0 || (m->status & DELETED))
		return senderr("no such message");
	cnt = atoi(lines);
	if(cnt < 0)
		return senderr("TOP bad args");
	sendok("");
	m_printcrnl(m, &out, cnt);
	sendcrnl(".");
	return 0;
}

static int
uidlcmd(char *arg)
{
	int n;
	message *m;
	uchar digest[SHA1dlen];
	char b64buf[2*SHA1dlen];

	if(*arg == 0){
		sendok("");
		n = 1;
		for(m = mlist; m; m = m->next){
			if(!(m->status & DELETED)){
				sha1((uchar*)s_to_c(m->body), m->size, digest, 0);
				enc64(b64buf, sizeof(b64buf), digest, SHA1dlen);
				sendcrnl("%d %s", n, b64buf);
			}
			n++;
		}
		sendcrnl(".");
	} else {
		n = atoi(arg);
		m = pointtomsg(n);
		if(m == 0 || (m->status & DELETED))
			return senderr("no such message");
		sha1((uchar*)s_to_c(m->body), m->size, digest, 0);
		enc64(b64buf, sizeof(b64buf), digest, SHA1dlen);
		sendok("%d %s", n, b64buf);
	}
	return 0;
}

extern char*
nextarg(char *p)
{
	while(*p && *p != ' ' && *p != '\t')
		p++;
	while(*p == ' ' || *p == '\t')
		*p++ = 0;
	return p;
}
