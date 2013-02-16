#include <u.h>
#include <libc.h>
#include <mp.h>
#include <fcall.h>
#include <thread.h>
#include <9p.h>
#include <auth.h>
#include <authsrv.h>
#include <libsec.h>
#include "netssh.h"

Cipher *cryptos[1];
int debug;

void
usage(void)
{
	fprint(2, "usage: %s [file]\n", argv0);
	exits("usage");
}

void
main(int argc, char *argv[])
{
	Packet *p;
	char *ep, *np, *user;
	mpint *e, *n;
	int fd, m;
	char key[Maxrpcbuf], encpub[Maxrpcbuf];

	ARGBEGIN {
	default:
		usage();
	} ARGEND
	if (argc > 1)
		usage();

	user = getenv("user");
	if (argc == 0)
		fd = 0;
	else {
		fd = open(argv[0], OREAD);
		if (fd < 0)
			usage();
	}
	m = read(fd, key, Maxrpcbuf - 1);
	close(fd);
	key[m >= 0? m: 0] = 0;

	ep = strstr(key, " ek=");
	np = strstr(key, " n=");
	if (ep == nil || np == nil)
		sysfatal("bad key file\n");
	e = strtomp(ep+4, nil, 16, nil);
	n = strtomp(np+3, nil, 16, nil);
	p = new_packet(nil);
	add_string(p, "ssh-rsa");
	add_mp(p, e);
	add_mp(p, n);
	if ((m = enc64(encpub, Maxrpcbuf, p->payload, p->rlength-1)) < 0)
		sysfatal("base-64 encoding failed\n");
	print("ssh-rsa ");
	write(1, encpub, m);
	if (user)
		print(" %s\n", user);
}
