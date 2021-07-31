#include <u.h>
#include <libc.h>

#include "../ssh.h"
#include "server.h"

char *progname;

RSApriv	*host_key;

char *base = "/sys/lib/ssh/hostkey";

char name[256];

void
main(int argc, char **argv)
{
	int fd;
	int decimal;

	decimal = 0;
	fmtinstall('B', mpconv);
	ARGBEGIN{
	case 'd':
		decimal = 1;
		break;
	default:
		goto Usage;
	}ARGEND

	switch(argc){
	default:
	Usage:
		fprint(2, "usage: ssh_genkey [-d] [basename]\n");
		exits("usage");
	case 0:
		break;
	case 1:
		base = argv[0];
		break;
	}

	do {
		host_key = rsagen(1024, 6, 0);
	} while (mpsignif(host_key->pub.n) != 1024);

	snprint(name, sizeof name, "%s.secret", base);
	if ((fd = create(name, OWRITE, 0600)) < 0)
		error("%r: %s", name);
	printsecretkey(fd, host_key);
	close(fd);

	snprint(name, sizeof name, "%s.public", base);
	if ((fd = create(name, OWRITE, 0644)) < 0)
		error("%r: %s", name);
	printpublickey(fd, &host_key->pub);

	if(decimal){
		snprint(name, sizeof name, "%s.public10", base);
		if ((fd = create(name, OWRITE, 0644)) < 0)
			error("%r: %s", name);
		printdecpublickey(fd, &host_key->pub);
	}

	close(fd);
}
