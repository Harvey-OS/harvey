#include <u.h>
#include <libc.h>

enum
{
	CtrlD	= 4,
	View	= 0x80,
	Cr	= 13,
};

void	error(char *);
void	sendmsg(int, char *);
void	connect(int, int);
void	kill(int);
int	asyncconfig(char*, char*, char*, int, int, int, char*);
int	muxconfig(char*, char*, int, int, int);
void	dkconfig(int, char*, int, int, int);
char*	system(int, char*);

/*
 *  default is:
 *	dev == #h
 *	csc == 4
 *	chans == 16
 *	baud == 9600
 *	net == dk
 *	ws == 7 (2K)
 */

void
main(int argc, char *argv[])
{
	char brate[32];
	int baud;
	Dir dir;
	char *net;
	char *dev;
	int csc, chans, ws;
	int i, async, incon;
	char *cmd;

	csc = 0;
	chans = 0;
	cmd = 0;
	incon = 0;
	async = 0;
	net = 0;
	dev = 0;
	baud = 9600;
	ws = 7;
	ARGBEGIN{
	case 'C':
		cmd = ARGF();
		break;
	case 'a':
		async = 1;
		break;
	case 'i':
		incon = 1;
		break;
	case 'c':
		csc = atoi(ARGF());
		chans = atoi(ARGF());
		break;
	case 'b':
		baud = atoi(ARGF());
		break;
	case 'd':
		dev = ARGF();
		break;
	case 'n':	/* ignore it (for dk232config) */
		net = ARGF();
		break;
	case 'w':
		ws = atoi(ARGF());
		break;
	}ARGEND

	if(ws > 017){
		for(i=0; i<020; i++)
			if(ws == 1<<(i+4)){
				ws = i;
				break;
			}
	}
	if(ws < 0 || ws > 017){
		fprint(2, "illegal window size\n");
		exits("ws");
	}

	if(async){
		if(net == 0){
			if(dirstat("/net/dk/clone", &dir) == 0)
				net = "dk232";
			else
				net = "dk";
		}
		if(csc == 0)
			csc = 1;
		if(chans == 0)
			chans = 16;
		if(dev == 0)
			dev = "/dev/eia0";
		sprint(brate, "B%d", baud);
		asyncconfig(net, dev, brate, csc, chans, ws, cmd);
	} else if(incon){
		if(net == 0)
			net = "dk";
		if(csc == 0)
			csc = 1;
		if(chans == 0)
			chans = 16;
		if(dev == 0)
			dev = "#i";
		muxconfig(net, dev, csc, chans, ws);
	} else {
		if(net == 0)
			net = "dk";
		if(csc == 0)
			csc = 4;
		if(chans == 0)
			chans = 256;
		if(dev == 0)
			dev = "#h";
		muxconfig(net, dev, csc, chans, ws);
	}
	exits(0);
}

/*
 *  configure the datakit
 */
void
dkconfig(int cfd, char* net, int csc, int chans, int ws)
{
	char buf[64];

	sendmsg(cfd, "push dkmux");
	sprint(buf, "config %d %d restart %s %d", csc, chans, net, ws);
	sendmsg(cfd, buf);

	/*
	 *  fork a process to hold the device channel open forever
	 */
	switch(fork()){
	case -1:
		break;
	case 0:
		for(;;)
			sleep(60*1000);
		exits(0);
	default:
		break;
	}
	sprint(buf, "#k%s", net);
	bind(buf, "/net", MBEFORE);
	close(cfd);
}

/*
 *  connect to a file system over the serial line
 */
int
asyncconfig(char *net, char *dev, char *baud, int csc, int chans, int ws, char *cmd)
{
	int cfd, dfd;
	char buf[NAMELEN+3];

	sprint(buf, "%sctl", dev);
	cfd = open(buf, ORDWR);
	if(cfd < 0)
		error(buf);
	sendmsg(cfd, baud);

	dfd = open(dev, ORDWR);
	if(dfd < 0)
		error(dev);
	if(cmd)
		system(dfd, cmd);
	connect(dfd, cfd);
	close(dfd);
	sendmsg(cfd, "push async");

	dkconfig(cfd, net, csc, chans, ws);
	return 0;
}

/*
 *  connect to a file system over a multiplexed device
 */
int
muxconfig(char *net, char *dev, int csc, int chans, int ws)
{
	int cfd;
	char buf[NAMELEN+3];

	sprint(buf, "%s/ctl", dev);
	cfd = open(buf, ORDWR);
	if(cfd < 0)
		error(buf);

	dkconfig(cfd, net, csc, chans, ws);
	return 0;
}

void
sendmsg(int fd, char *msg)
{
	int n;

	n = strlen(msg);
	if(write(fd, msg, n) != n)
		error(msg);
}

/*
 *  print error and exit
 */
void
error(char *s)
{
	char buf[64];

	errstr(buf);
	fprint(2, "%s: %s: %s\n", argv0, s, buf);
	exits(0);
}

void
connect(int fd, int cfd)
{
	char xbuf[128];
	int i, pid, n, ctl;

	print("Connect to file system now, type ctrl-d when done.\n");
	print("(Use the view or down arrow key to send a break)\n");

	switch(pid = fork()) {
	case -1:
		error("fork failed");
	case 0:
		for(;;) {
			n = read(fd, xbuf, sizeof(xbuf));
			if(n < 0) {
				errstr(xbuf);
				print("[remote read error (%s)]\n", xbuf);
				for(;;);
			}
			for(i = 0; i < n; i++)
				if(xbuf[i] == Cr)
					xbuf[i] = ' ';
			write(1, xbuf, n);
		}
	default:
		ctl = open("/dev/consctl", OWRITE);
		if(ctl < 0)
			error("opening consctl");
		write(ctl, "rawon", 5);

		for(;;) {
			read(0, xbuf, 1);
			switch(xbuf[0]) {
			case CtrlD:
				kill(pid);
				close(ctl);
				return;
			case View:	/* send a break */
				write(cfd, "k250", 4);
				break;
			default:
				n = write(fd, xbuf, 1);
				if(n < 0) {
					errstr(xbuf);
					kill(pid);
					close(ctl);
					print("[remote write error (%s)]\n", xbuf);
				}
			}
		}
	}
}

void
kill(int pid)
{
	char xbuf[32];
	int f;

	sprint(xbuf, "/proc/%d/note", pid);
	f = open(xbuf, OWRITE);
	write(f, "die", 3);
	close(f);
}

/*
 *  run a command with the network connection as standard IO
 */
char *
system(int fd, char *cmd)
{
	int pid;
	int p;
	static Waitmsg msg;

	switch(pid = fork()){
	case -1:
		perror("con");
		return "fork failed";
	case 0:
		dup(fd, 0);
		dup(fd, 1);
		close(fd);
		execl("/bin/rc", "rc", "-c", cmd, 0);
		perror("dkconfig");
		exits("exec");
		return 0;
	default:
		for(;;){
			p = wait(&msg);
			if(p < 0)
				return "lost child";
			if(p == pid)
				return msg.msg;	
		}
		break;
	}
	return 0;
}
