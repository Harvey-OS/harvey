#include	"all.h"

/*
 * driver for wire-wrap machine
 */


#define TIMN	5		/* max number of timeouts */
#define TIMS	3000		/* number of msec's for timeout */

static int	tfd;		/* console (`terminal') */
static int	mfd;		/* serial line to machine */
static int	timeout;

/*
 * send to device
 */
static void
wwsend(int cmd, int d0, int d1)
{
	char cbuf[4], *cp;

	cp = cbuf;
	*cp++ = CMD;
	*cp++ = d0;
	*cp++ = d1;
	*cp   = cmd;
	if(write(mfd, cbuf, 4) < 0){
		perror(mfile);
		exits("wwsend");
	}
}


/*
 * read char from device, or timeout
 */
static int
rdmach(void)
{
	int timcnt;
	uchar c;

	timcnt = 0;
	timeout = 0;
	alarm(TIMS);
	while(read(mfd, &c, 1) <= 0){
		if(!timeout)
			return -1;
		if(timcnt++ > TIMN){
			print("timeout - check wire wrap machine power\n");
			exits("timeout");
		}
		wwsend(SENSTAT, 0, 0);
		alarm(TIMS);
	}
	alarm(0);
	return c;
}

/*
 * wait for command response
 * mask is for status bits to wait for
 * return new status
 * return -1 if error
 */
static int
cmdwait(int mask)
{
	int c;

	mask |= RERR|ILLCMD|EMICRO;
	for(;;){
		while(rdmach() != STAT) ;
		c = rdmach();
		if(c&mask)
			break;
	}
	return (c & (RERR|ILLCMD|EMICRO)) ? -1 : c;
}


/*
 * timeout
 */
static int
alarmed(void *ureg, char *note)
{
	USED(ureg);
	if(strcmp(note, "alarm") != 0)
		return 0;
	++timeout;
	return 1;
}


/*
 * write command to device
 * return status after command completion
 * return -1 if error
 */
int
wwcmd(int cmd, int d0, int d1)
{
	int stat;

	wwsend(cmd, d0, d1);
	stat = cmdwait(CACK);
	if(stat > 0 && !(stat & MDONE) && (cmd == LOADGOX || cmd == LOADGOY))
		stat = cmdwait(MDONE);
	return stat;
}


/*
 * encode integer num into buf[2]
 */
static void
smag(int num, char *buf)
{
	int mflg;

	if(num < 0){
		num = -num;
		mflg = 0100;
	}else
		mflg = 0;
	*buf++ = num & 0177;
	*buf = ((num & 017600) >> 7) | mflg;
}

/*
 * move the pointer
 * return status or -1
 */
int
wwmove(int delx, int dely)
{
	char data[2];

	smag(dely, data);
	wwcmd(LOADY, data[0], data[1]);
	smag(delx, data);
	absx += delx;
	absy += dely;
	return wwcmd(LOADGOX, data[0], data[1]);
}


/*
 * read character from terminal
 */
int
rdkey(void)
{
	uchar c;
	int n;

	n = read(tfd, &c, 1);
	return (n > 0) ? c : EOFC;
}


/*
 * initialise device
 */
void
wwinit(void)
{
	int cfd;
	char cfile[3*NAMELEN];

	mfd = open(mfile, ORDWR);
	if(mfd < 0){
		perror(mfile);
		exits("wwinit");
	}
	sprint(cfile, "%sctl", mfile);
	cfd = open(cfile, ORDWR);
	if(cfd < 0){
		perror(cfile);
		exits("wwinit-ctl");
	}
	fprint(cfd, "b300");
	close(cfd);
	tfd = open("/dev/cons", OREAD);
	if(tfd < 0){
		perror("/dev/cons");
		exits("wwinit");
	}
	sprint(cfile, "/dev/consctl");
	cfd = open(cfile, OWRITE);
	if(cfd < 0){
		perror(cfile);
		exits("wwinit-cons");
	}
	fprint(cfd, "rawon");
	atnotify(alarmed, 1);
}
