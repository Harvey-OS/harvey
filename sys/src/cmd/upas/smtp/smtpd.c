#include "common.h"
#include "smtpd.h"

char *me;
char *him;

typedef struct Link Link;
typedef struct List List;

struct Link {
	Link *next;
	String *p;
};

struct List {
	Link *first;
	Link *last;
};

List senders;
List rcvers;
Biobuf bin;

int debug;

void
main(int argc, char **argv)
{
	ARGBEGIN{
	case 'd':
		debug++;
		break;
	default:
		fprint(2, "usage: smtpd [-d]\n");
		exits("usage");
	}ARGEND

	Binit(&bin, 0, OREAD);

	if(debug){
		close(2);
		open("/sys/log/smtpd", OWRITE);
		fprint(2, "%d smtpd %s\n", getpid(), thedate());
	}

	me = sysname_read();
	sayhi();
	parseinit();
	yyparse();
	exits(0);
}

static void
listfree(List *l)
{
	Link *lp;
	Link *next;

	for(lp = l->first; lp; lp = next){
		next = lp->next;
		s_free(lp->p);
		free(lp);
	}
	l->first = l->last = 0;
}

static void
listadd(List *l, String *path)
{
	Link *lp;

	lp = (Link *)malloc(sizeof(Link));
	lp->p = path;
	lp->next = 0;

	if(l->last)
		l->last->next = lp;
	else
		l->first = lp;
	l->last = lp;
}

#define	SIZE	4096
#define	DOTDOT	(&fmt+1)
int
reply(char *fmt, ...)
{
	char buf[SIZE], *out;
	int n;

	out = doprint(buf, buf+SIZE, fmt, DOTDOT);
	n = (long)(out-buf);
	if(debug)
		write(2, buf, n);
	write(1, buf, n);
	return n;
}

void
reset(void)
{
	reply("250 ok\r\n");
}

void
sayhi(void)
{
	char *dom;

	dom = domainname_read();
	reply("220 %s SMTP\r\n", dom[0] ? dom : me);
}

void
syntaxerr(void)
{
	reply("501 illegal command\r\n");
}

void
hello(String *himp)
{
	him = s_to_c(himp);
	reply("250 you are %s\r\n", him);
}

int
badchar(char *p)
{
	return strchr(p, '\'') != 0;
}

void
sender(String *path)
{
	if(him == 0){
		reply("503 Start by saying HELO, please.\r\n", s_to_c(path));
		return;
	}
	if(badchar(s_to_c(path))){
		reply("503 Bad character in sender address %s.\r\n", s_to_c(path));
		return;
	}
	listadd(&senders, path);
	reply("250 sender is %s\r\n", s_to_c(path));
}

void
receiver(String *path)
{
	if(him == 0){
		reply("503 Start by saying HELO, please\r\n");
		return;
	}
	listadd(&rcvers, path);
	reply("250 receiver is %s\r\n", s_to_c(path));
}

void
quit(void)
{
	reply("221 Successful termination\r\n");
	exits(0);
}

void
turn(void)
{
	reply("502 TURN unimplemented\r\n");
}

void
noop(void)
{
	reply("250 Stop wasting my time!\r\n");
}

void
help(String *cmd)
{
	if(cmd)
		s_free(cmd);
	reply("250 Read rfc821 and stop wasting my time\r\n");
}

void
verify(String *path)
{
	process *pp;
	String *cmd;
	char buf[2][128];
	int i;
	char *cp;

	if(badchar(s_to_c(path))){
		reply("503 Bad character in address %s.\r\n", s_to_c(path));
		return;
	}
	cmd = s_new();
	s_append(cmd, "mail -x '");
	s_append(cmd, s_to_c(path));
	s_append(cmd, "'");

	pp = proc_start(s_to_c(cmd), (stream *)0, outstream(),  (stream *)0, 1, 0);
	if (pp == 0) {
		reply("450 We're busy right now, try later\r\n");
		s_free(cmd);
		return;
	}

	buf[0][0] = buf[1][0] = 0;
	for(i = 0; cp = Brdline(pp->std[1]->fp, '\n'); i ^= 1){
		cp[Blinelen(pp->std[1]->fp)-1] = 0;
		strncpy(buf[i], cp, sizeof(buf[i])-1);
		buf[i][sizeof(buf[i])-1] = 0;

		/* print previous line */
		if(buf[i^1][0]){
			cp = strchr(buf[i^1], '\n');
			if(cp)
				*cp = 0;
			if(strchr(buf[i^1], ':'))
				reply("250-%s\r\n", buf[i^1]);
			else
				reply("250-<%s@%s>\r\n", buf[i^1], me);
		}
	}
	if(buf[i^1][0]){
		cp = strchr(buf[i^1], '\n');
		if(cp)
			*cp = 0;
		if(strchr(buf[i^1], ':'))
			reply("250 %s %s\r\n", s_to_c(path), buf[i^1]);
		else
			reply("250 <%s@%s>\r\n", buf[i^1], me);
	}
	proc_wait(pp);
	proc_free(pp);
	s_free(cmd);
}

/*
 *  get a line that ends in crnl or cr, turn terminating crnl into a nl
 *
 *  return 0 on EOF
 */
static int
getcrnl(char *buf, int n, Biobuf *fp)
{
	int c;
	char *ep;
	char *bp;

	bp = buf;
	ep = bp + n - 1;
	while(bp != ep){
		c = Bgetc(fp);
		if(debug)
			fprint(2, "%c", c);
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
				if(debug)
					fprint(2, "%c", c);
				*bp++ = '\n';
				*bp = 0;
				return bp-buf;
			}
			Bungetc(fp);
			c = '\r';
			break;
		case '\n':
			*bp++ = '\n';
			*bp = 0;
			return bp-buf;
		}
		*bp++ = c;
	}
	*bp = 0;
	return bp-buf;
}

void
data(void)
{
	process *pp;
	String *cmd;
	String *err;
	int status = 0;
	Link *l;
	char *cp;
	char *ep;
	char buf[4096];
	int sol, n;

	if(senders.last == 0){
		reply("503 Data without MAIL FROM:\r\n");
		return;
	}
	if(rcvers.last == 0){
		reply("503 Data without RCPT TO:\r\n");
		return;
	}

	/*
	 *  set up mail command
	 */
	cmd = s_new();
	s_append(cmd, "upasname='");
	s_append(cmd, s_to_c(senders.first->p));
	s_append(cmd, "' /bin/upas/sendmail -r");
	for(l = rcvers.first; l; l = l->next){
		s_append(cmd, " '");
		s_append(cmd, s_to_c(l->p));
		s_append(cmd, "'");
	}

	/*
	 *  start mail process
	 */
	pp = proc_start(s_to_c(cmd), instream(), 0, outstream(), 1, 0);
	if(pp == 0) {
		reply("450 We're busy right now, try later\n");
		return;
	}
	s_free(cmd);

	reply("354 Input message; end with <CRLF>.<CRLF>\r\n");

	/*
	 *  give up after 45 minutes, something has to be really wrong
	 *  to take longer than that.
	 */
	alarm(45*60*1000);

	/*
	 *  read first line.  If it is a 'From ' line, leave it.  Otherwise,
	 *  add one.
	 */
	n = getcrnl(buf, sizeof(buf), &bin);
	cp = buf;
	if(n==0 || strncmp(cp, "From ", 5)!=0)
		Bprint(pp->std[0]->fp, "From %s %s\n", s_to_c(senders.first->p),
			thedate());

	/*
	 *  pass message to mailer.  take care of '.' escapes.
	 */
	pipesig(&status);	/* set status to 1 on write to closed pipe */
	for(sol = 1; status == 0;){
		if(cp == 0){
			/* premature EOF */
			proc_kill(pp);
			status = 1;
			break;
		}
		if(sol && *cp=='.'){
			/* '.'s at the start of line is an escape */
			cp++;
			n--;
			if(*cp=='\n')
				break;
		}
		sol = cp[n-1] == '\n';
		if(Bwrite(pp->std[0]->fp, cp, n) < 0)
			status = 1;
		n = getcrnl(buf, sizeof(buf), &bin);
		if(n == 0)
			break;
		cp = buf;
	}
	alarm(0);

	if(Bflush(pp->std[0]->fp) < 0)
		status = 1;
	stream_free(pp->std[0]);
	pp->std[0] = 0;

	/*
	 *  read any error messages
	 */
	err = s_new();
	while(s_read_line(pp->std[2]->fp, err))
		;
	if(debug){
		fprint(2, "%d status %d\n", getpid(), status);
		if(*s_to_c(err))
			fprint(2, "%d error %s\n", getpid(), s_to_c(err));
	}
	status |= proc_wait(pp);
	proc_free(pp);

	/*
	 *  if process terminated abnormally, send back error message
	 */
	if(status){
		for(cp = s_to_c(err); ep = strchr(cp, '\n'); cp = ep){
			*ep++ = 0;
			reply("450-%s\r\n", cp);
		}
		reply("450 mail process terminated abnormally\r\n");
	} else
		reply("250 sent\r\n");
	s_free(err);

	pipesigoff();

	listfree(&senders);
	listfree(&rcvers);
}
