#include <alef.h>
#include "ppp.h"
#include "compress.h"
#define CLASS(p) ((*(byte *)(p))>>6)

PPPstate	s;

intern int
dialdev(byte *line, int baud)
{
	int data, ctl;
	byte file[32];
	byte cmd[32];

	data = open(line, ORDWR);
	if(data < 0)
		fatal(line);
	sprint(file, "%sctl", line);
	ctl = open(file, ORDWR);
	if(ctl < 0)
		fatal(file);
	sprint(cmd, "b%d", baud);
	if(write(ctl, cmd, strlen(cmd)) != strlen(cmd))
		fatal("writing ctl\n");
	close(ctl);
	return data;
}

intern void
usage(void)
{
	fatal("usage: ppp [-d] [-b<speed>] [-i myipaddr] [-p peeripaddr] [dialstring]");
}

int
parseip(byte *to, byte *from)
{
	int i;
	byte *p;

	p = from;
	memset(to, 0, 4);
	for(i = 0; i < 4 && *p; i++){
		to[i] = strtoui(p, &p, 0);
		if(*p == '.')
			p++;
	}

	switch(CLASS(to)){
	case 0:	/* class A - 1 byte net */
	case 1:
		if(i == 3){
			to[3] = to[2];
			to[2] = to[1];
			to[1] = 0;
		} else if (i == 2){
			to[3] = to[1];
			to[1] = 0;
		}
		break;
	case 2:	/* class B - 2 byte net */
		if(i == 3){
			to[3] = to[2];
			to[2] = 0;
		}
		break;
	}
	return 0;
}

void
ipencode(void)
{
	byte	buf[Maxip], *bufp;
	usint	prtcl;
	int	n;

	while((n = read(ipfd, buf, sizeof(buf))) > 0) {
		bufp = buf + ETHER_HDR;
		n -= ETHER_HDR;
		if(s.ipcp.options & (1 << IPCompressionPrtcl))
			(prtcl, bufp, n) = compress(&s, bufp);
		else
			prtcl = PRTCL_IP;
		DPRINT("Send Size = %d\n", n);
		while(n > s.lcp.peermaxsize) {
			s.pppencode(prtcl, bufp, s.lcp.peermaxsize, 0);
			n -= s.lcp.peermaxsize;
			bufp += s.lcp.peermaxsize;
		}
		s.pppencode(prtcl, bufp, n, 1);
	}
	exits(nil);
}

void
main(int argc, byte **argv)
{
	int baud, opt, fd;
	byte	myipaddr[4], peeripaddr[4];

	fd = -1;
	baud = 9600;
	memset(myipaddr, 0, sizeof(myipaddr));
	memset(peeripaddr, 0, sizeof(peeripaddr));
	for(opt = 1; opt < argc && *argv[opt] == '-';) {
		switch(*(argv[opt++]+1)) {
		case 'd':
			++debug;
			break;
		case 'b':
			baud = atoi(argv[opt++]);
			break;
		case 'i':
			parseip(myipaddr, argv[opt++]);
			break;
		case 'p':
			parseip(peeripaddr, argv[opt++]);
			break;
		default:
			usage();
		}
	}
	if(argv[opt] != nil) {
		if(*argv[opt] == '/')
			fd = dialdev(argv[opt], baud);
		else
			fd = dial(argv[opt], nil, nil, nil);
	} else if(opt == argc)
		fd = 0;
	else
		usage();
	if(fd < 0)
		fatal(argv[opt]);

	s.initppp(fd, myipaddr, peeripaddr);
	proc doalarms();
	while(s.allset == 0)
		sleep(1000);
	ipconfig(s.ipcp.myipaddr, 0);
	proc ipencode();
	exits(nil);
}
