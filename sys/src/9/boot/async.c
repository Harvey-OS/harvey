#include <u.h>
#include <libc.h>
#include <../boot/boot.h>

static void	configasync(char*, char*);
static void	connect(int, int);
static void	kill(int);

void
config9600(Method *mp)
{
	configasync("B9600", mp->arg);
}

int
auth9600(void)
{
	return dkauth();
}

int
connect9600(void)
{
	return dkconnect();
}

void
config19200(Method *mp)
{
	configasync("B19200", mp->arg);
}

int
auth19200(void)
{
	return dkauth();
}

int
connect19200(void)
{
	return dkconnect();
}


static void
configasync(char *baud, char *dev)
{
	int cfd, dfd;
	char eianame[NAMELEN];

	if(*sys == '/' || *sys == '#')
		dev = sys;
	else if(dev == 0)
		dev = "#t/eia0";
	sprint(eianame, "%sctl", dev);
	cfd = open(eianame, ORDWR);
	if(cfd < 0)
		fatal(eianame);
	sendmsg(cfd, baud);

	sprint(eianame, "%s", dev);
	dfd = open(eianame, ORDWR);
	if(dfd < 0)
		fatal(eianame);
	connect(dfd, cfd);
	close(dfd);
	sendmsg(cfd, "push async");
	sendmsg(cfd, "push dkmux");
	sendmsg(cfd, "config 1 16 restart dk 4");
	close(cfd);
}

enum
{
	CtrlD	= 0x4,
	CtrlE	= 0x5,
	CtrlO	= 0xf,
	Cr	= 13,
	View	= 0x80,
};

static void
connect(int fd, int cfd)
{
	char xbuf[128];
	int i, pid, n, ctl;

	print("Connect to file system now, type ctrl-d when done.\n");
	print("...(Use the view or down arrow key to send a break)\n");
	print("...(Use ctrl-e to set even parity or ctrl-o for odd)\n");

	switch(pid = fork()) {
	case -1:
		fatal("fork failed");
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
		break;
	default:
		ctl = open("#c/consctl", OWRITE);
		if(ctl < 0)
			fatal("opening consctl");
		write(ctl, "rawon", 5);

		for(;;){
			read(0, xbuf, 1);
			switch(xbuf[0]&0xff) {
			case CtrlD:	/* done */
				kill(pid);
				close(ctl);
				return;
			case CtrlE:	/* set even parity */
				write(cfd, "pe", 2);
				break;
			case CtrlO:	/* set odd parity */
				write(cfd, "po", 2);
				break;
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

static void
kill(int pid)
{
	char xbuf[32];
	int f;

	sprint(xbuf, "#p/%d/note", pid);
	f = open(xbuf, OWRITE);
	write(f, "die", 3);
	close(f);
}
