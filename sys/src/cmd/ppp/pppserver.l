#include <alef.h>
#include "ppp.h"
#include "compress.h"
#define CLASS(p) ((*(byte *)(p))>>6)

intern PPPstate states[32];
intern int ipbase;

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

intern void
usage(void)
{
	fatal("usage: pppserver [-d] nlines myip baseip baseseriallineno (baudratelist)");
}

void
ipmuxencode(void)
{
	byte	buf[Maxip], *bufp;
	usint	prtcl;
	int	n;
	PPPstate *s;
	Ip	*ip;

	rfork(RFNOTEG);
	while((n = read(ipfd, buf, sizeof(buf))) > 0) {
		bufp = buf + ETHER_HDR;
		n -= ETHER_HDR;
		ip = (Ip *) bufp;
		s = &states[ip->dst[3] - ipbase];
		if(s->allset == 0)
			continue;
		if(s->ipcp.options & (1 << IPCompressionPrtcl))
			(prtcl, bufp, n) = compress(s, bufp);
		else
			prtcl = PRTCL_IP;
		DPRINT("Send Size = %d\n", n);
		while(n > s->lcp.peermaxsize) {
			s->pppencode(prtcl, bufp, s->lcp.peermaxsize, 0);
			n -= s->lcp.peermaxsize;
			bufp += s->lcp.peermaxsize;
		}
		s->pppencode(prtcl, bufp, n, 1);
	}
	exits(nil);
}

void
main(int argc, byte **argv)
{
	int	baud, opt, fd, n, nuart, i;
	byte	baseipaddr[4], myipaddr[4], str[150];

	for(opt = 1; opt < argc && *argv[opt] == '-';) {
		switch(*(argv[opt++]+1)) {
		case 'd':
			++debug;
			break;
		default:
			usage();
		}
	}
	if(argc - opt < 4)
		usage();
	n = atoi(argv[opt++]);
	parseip(myipaddr, argv[opt++]);
	parseip(baseipaddr, argv[opt++]);
	nuart = atoi(argv[opt++]);

	ipbase = baseipaddr[3];

	for(i = 0; i < n && i < nelem(states); i++) {
		if(argv[opt] != nil)
			baud = atoi(argv[opt++]);
		else
			baud = 9600;
		sprint(str, "/dev/eia%d", nuart + i);
		fd = dialdev(str, baud);
		if(fd < 0)
			continue;
		states[i].initppp(fd, myipaddr, baseipaddr);
		baseipaddr[3]++;
	}
	ipconfig(myipaddr, 1);
	proc ipmuxencode();
	proc doalarms();
	exits(nil);
}
