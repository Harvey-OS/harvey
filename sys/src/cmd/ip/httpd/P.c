/* PalmPilot gateway to chess server  (ehg, lorenz) */
#include <u.h>
#include <libc.h>
#include <bio.h>
#include <libsec.h>
#include "httpd.h"
#include "httpsrv.h"

char *P2 = "tcp!netlib2.cs.bell-labs.com!575";
enum{ CHAN_IMMEDIATE=0, CHAN_NEW, CHAN_START=12000 };

Biobuf	bserv, bservr;

void
main(int argc, char **argv)
{
	HConnect *c;
	Hio *hout;
	int ch, datafd, n_to, n_from;
	char *s;
	char addr[200];
	uchar bin[3*1024];

	c = init(argc, argv);
	hout = &c->hout;
	if(c->req.search == nil){
		hfail(c, HSyntax);
		exits("failed");
	}
	alarm(120000);

	n_to = strlen(c->req.search);
	if(n_to > 4*(sizeof(bin)-1)/3)
		n_to = 4*(sizeof(bin)-1)/3;
	n_to = dec64(bin, sizeof bin, c->req.search, n_to);
	strncpy(addr, P2, sizeof addr);
	if(bin[0] != CHAN_IMMEDIATE && bin[0] != CHAN_NEW){
		s = strchr(addr+4, '!');
		if(s == nil){
			hfail(c, HInternal, "P2 addr");
			exits("failed");
		}
		s++;
		snprint(s, sizeof addr - (s-addr), "%d", CHAN_START+bin[0]-(CHAN_NEW+1));
	}
	datafd = dial(addr, nil, nil, nil);
	if(datafd < 0){
		hfail(c, HNotFound, addr);
		exits("failed");
	}
	Binit(&bserv, datafd, OWRITE);

	/* copy the incoming request */
	Bwrite(&bserv, (char*)bin, n_to);
	Bterm(&bserv);

	/* copy reply */
	Binit(&bservr, datafd, OREAD);
	n_from = 0;
	while((ch = Bgetc(&bservr)) != Beof){
		hputc(hout, ch);
		n_from++;
	}
	hflush(hout);
	exits(nil);
}
