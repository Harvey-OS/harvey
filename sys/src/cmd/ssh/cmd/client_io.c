#include <u.h>
#include <libc.h>

#include "../ssh.h"
#include "client.h"

static int raw;
static int consctl = -1;
static char* system(int, char*);
static int outfd1=1, outfd2=2;
static int nltocr;

/*
 *  turn keyboard raw mode on
 */
static void
rawon(void)
{
	if(raw)
		return;
	if(consctl < 0)
		return;
	write(consctl, "rawon", 5);
	raw = 1;
}

/*
 *  turn keyboard raw mode off
 */
static void
rawoff(void)
{
	if(raw == 0)
		return;
	if(consctl < 0)
		return;
	write(consctl, "rawoff", 6);
	raw = 0;
}

/*
 *  control menu
 */
#define STDHELP	"\t(q)uit, toggle printing (r)eturns, (.)continue, (!cmd)\n"

int
menu(int net)
{
	char buf[1024];
	long n;
	int done;
	int wasraw;

	wasraw = raw;
	if(wasraw)
		rawoff();

	fprint(2, ">>> ");
	for(done = 0; !done; ){
		n = read(0, buf, sizeof(buf)-1);
		if(n <= 0)
			return -1;
		buf[n] = 0;
		switch(buf[0]){
		case '!':
			print(buf);
			system(net, buf+1);
			print("!\n");
			done = 1;
			break;
		case '.':
			done = 1;
			break;
		case 'q':
			return -1;
			break;
		case 'r':
			crstrip = 1-crstrip;
			done = 1;
			break;
		default:
			fprint(2, STDHELP);
			break;
		}
		if(!done)
			fprint(2, ">>> ");
	}

	if(wasraw)
		rawon();
	else
		rawoff();
	return 0;
}

static char*
syserr(void)
{
	static char err[ERRLEN];
	errstr(err);
	return err;
}

static int
wasintr(void)
{
	return strcmp(syserr(), "interrupted") == 0;
}

static int
msgwrite(char *buf, int n)
{
	Packet *packet;

	packet = (Packet *)mmalloc(sizeof(Packet)+8+n);
	packet->length = 0;
	packet->pos = packet->data;
	if (n == 0) {
		packet->type = SSH_CMSG_EOF;
		putpacket(packet, c2s);
		return 0;
	}
	packet->type = SSH_CMSG_STDIN_DATA;
	putstring(packet, buf, n);
	if (putpacket(packet, c2s))
		return -1;
	return n;
}

/*
 *  run a command with the network connection as standard IO
 */
static char *
system(int fd, char *cmd)
{
	int pid;
	int p;
	static Waitmsg msg;
	int pfd[2];
	int n;
	int wasconsctl;
	char buf[4096];

	if(pipe(pfd) < 0){
		perror("pipe");
		return "pipe failed";
	}
	outfd1 = outfd2 = pfd[1];

	wasconsctl = consctl;
	close(consctl);
	consctl = -1;
	switch(pid = fork()){
	case -1:
		perror("con");
		return "fork failed";
	case 0:
		close(pfd[1]);
		dup(pfd[0], 0);
		dup(pfd[0], 1);
		close(fd);
		close(pfd[0]);
		if(*cmd)
			execl("/bin/rc", "rc", "-c", cmd, 0);
		else
			execl("/bin/rc", "rc", 0);
		perror("con");
		exits("exec");
		break;
	default:
		close(pfd[0]);
		while((n = read(pfd[1], buf, sizeof(buf))) > 0)
			if(msgwrite(buf, n) != n)
				break;
		p = wait(&msg);
		outfd1 = 1;
		outfd2 = 2;
		close(pfd[1]);
		if(p < 0 || p != pid)
			return "lost child";
		break;
	}
	if(wasconsctl >= 0){
		consctl = open("/dev/consctl", OWRITE);
		if(consctl < 0)
			error("cannot open consctl");
	}
	return msg.msg;
}

/*
 * The user has requested to quit, or we got end of file
 * on standard input.  Either way, we need to interrupt
 * the current read, so that closing the file descriptors will
 * be noticed.
 */
static void
teardown(void)
{
	msgwrite("", 0);
	close(dfdin);
	dfdin = dfdout = -1;
	postnote(PNGROUP, getpid(), "hangup");
	exits(0);
}

static int
doinput(void) {
	int n;
	char buf[1024];
	int pid;
	char *p, *ep;
	int eofs;

	if ((pid = rfork(RFMEM|RFPROC|RFNOWAIT)) < 0)
		error("fork: %r");
	if (pid)
		return pid;

	debug(DBG_PROC, "doinput started\n");
	/* Child */

	if(interactive){
		if ((consctl=open("/dev/consctl", OWRITE)) < 0)
			error("Can't turn off echo");
		if(cooked==0)
			rawon();
	}else
		consctl = -1;

	eofs = 0;
	for(;;){
		n = read(0, buf, sizeof(buf));
		if(n < 0){
			if(wasintr()){
				if(cooked){
					buf[0] = 0x7f;
					n = 1;
				} else
					continue;
			} else
				goto End;
		}
		if(n == 0){
			if(++eofs > 32)
				goto End;
		} else
			eofs = 0;
		if(interactive && usemenu && n && memchr(buf, 0x1c, n)) {
			if(menu(dfdin) < 0)
				teardown();
			continue;
		}

		if(cooked && n==0){
			buf[0] = 0x4;
			n = 1;
		}
		if(nltocr){
			ep = buf+n;
			for(p = buf; p < ep; p++)
				switch(*p){
				case '\r':
					*p = '\n';
					break;
				case '\n':
					*p = '\r';
					break;
				}
		}

		if(msgwrite(buf, n) != n)
			break;
	}

	if(consctl >= 0)
		close(consctl);

End:
	teardown();

	/* not reached, for ken... */
	return -1;
}

static int
dowinchange(void)
{
	int pid;

	if ((pid = rfork(RFMEM|RFPROC|RFNOWAIT)) < 0)
		error("fork: %r");
	if (pid)
		return pid;

	debug(DBG_PROC, "dowinchange started\n");
	/* Child */

	/* report changes */
	for(;;){
		if(readgeom() < 0)
			break;
		window_change();
	}
	_exits(0);
	return -1;	/* for compiler */
}

void
kill(int pid) {
	int fd;
	char proc[64];

	if(pid < 0)
		return;

	sprint(proc, "/proc/%d/ctl", pid);
	fd = open(proc, OWRITE);
	debug(DBG_PROC, "echo kill>/proc/%d/ctl\n", pid);
	fprint(fd, "kill\n");
	close(fd);
}

void
run_shell(char *argv[]) {
	Packet *packet;
	char *buf;
	uchar *p;
	int nbuf;
	int n, winchangepid, inputpid;

	winchangepid = -1;

	rfork(RFNOTEG);
	nbuf = 8192+2;
	buf = mmalloc(8192+2);
	if (*argv == 0)
		requestpty = 1;
	if (requestpty)
		request_pty();
	packet = (Packet*)mmalloc(nbuf+64);
	packet->length = 0;
	packet->pos = packet->data;
	if (*argv) {
		packet->type = SSH_CMSG_EXEC_CMD;
		buf[0] = 0;
		while (*argv) {
			strncat(buf, *argv++, nbuf - strlen(buf));
			strncat(buf, " ", nbuf - strlen(buf));
		}
		if (buf[strlen(buf)-1] == ' ') buf[strlen(buf)-1] = 0;
		putstring(packet, buf, strlen(buf));
		if (verbose) {
			fprint(2, "Executing command: ");
			write(2, buf, strlen(buf));
			write(2, "\n", 1);
		}
	} else {
		if (verbose)
			fprint(2, "Starting shell\n");
		packet->type = SSH_CMSG_EXEC_SHELL;
	}
	if (putpacket(packet, c2s))
		exits("connection");

	inputpid = doinput();

	/*
	 * Only dowinchange() if requesting a pty, otherwise
	 * we don't know cheight, cwidth, and don't care.
	 */
	if (requestpty) 
		winchangepid = dowinchange();
	while(1) {
		packet = getpacket(s2c);
		if (packet == 0) break;
		if (packet->length > nbuf){
			/*
			 * The smallest allowed maximum we can place on message length
			 * is a quarter megabyte, so we grow the buffer as necessary
			 * rather than allocate a quarter meg right off.
			 */
			nbuf = packet->length + 64;
			buf = realloc(buf, nbuf);
			if(buf == nil){
				error("out of memory reallocing %d", nbuf);
				abort();
			}
		}
		switch (packet->type) {
		case SSH_SMSG_EXITSTATUS:
			debug(DBG, "Exits(0)\n");
			kill(inputpid);
			if (requestpty)
				kill(winchangepid);
			if (verbose)
				fprint(2, "Remote process exited\n");
			exits(0);
		case SSH_MSG_DISCONNECT:
			getstring(packet, buf, nbuf);
			debug(DBG, "Exits(%s)\n", buf);
			if (verbose)
				fprint(2, "Remote side disconnected\n");
			kill(inputpid);
			if (requestpty)
				kill(winchangepid);
			exits(buf);
		case SSH_SMSG_STDOUT_DATA:
			p = packet->data;
			debug(DBG, "Stdout pkt %d len %d\n",
				packet->length, (p[0]<<24)|(p[1]<<16)|(p[2]<<8)|p[3]);
			n = getstring(packet, buf, nbuf);
			if (crstrip) {
				int i, m;
				char c;

				m = 0;
				for (i = 0; i < n; i++) {
					if ((c = buf[i]) != '\r') buf[m++] = c;
				}
				buf[m] = '\0';
				n = m;
			}
			write(outfd1, buf, n);
			break;
		case SSH_SMSG_STDERR_DATA:
			n = getstring(packet, buf, nbuf);
			if (crstrip) {
				int i, m;
				char c;

				m = 0;
				for (i = 0; i < n; i++) {
					if ((c = buf[i]) != '\r') buf[m++] = c;
				}
				buf[m] = '\0';
				n = m;
			}
			write(outfd2, buf, n);
			break;
		default:
			error("Unknown message type %d", packet->type);
		}
		mfree(packet);
	}
}
