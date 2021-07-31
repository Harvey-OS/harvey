#include "ssh.h"

char *base = "/sys/lib/ssh/hostkey";
RSApriv *key;

void
usage(void)
{
	fprint(2, "usage: aux/ssh_genkey [basename]\n");
	fprint(2, "\tdefault basename is /sys/lib/ssh/hostkey\n");
	exits("usage");
}

void
printsecret(int fd)
{
	fprint(fd, "%d %B %B %B %B %B %B %B %B",
		mpsignif(key->pub.n), key->pub.ek,
		key->dk, key->pub.n, key->p, key->q,
		key->kp, key->kq, key->c2);
}

void
printsecretfactotum(int fd)
{
	fprint(fd, "key proto=sshrsa size=%d ek=%B !dk=%B n=%B !p=%B !q=%B !kp=%B !kq=%B !c2=%B",
		mpsignif(key->pub.n), key->pub.ek,
		key->dk, key->pub.n, key->p, key->q,
		key->kp, key->kq, key->c2);
}

/*
void
printpublic(int fd)
{
	fprint(fd, "%d %B %B\n", mpsignif(key->pub.n), key->pub.ek, key->pub.n);
}
*/

void
printpublic(int fd)
{
	fprint(fd, "%d %.10B %.10B\n", mpsignif(key->pub.n), key->pub.ek, key->pub.n);
}

void
writefile(char *fmt, int perm, void (*fn)(int))
{
	int fd;
	char *s;

	s = smprint(fmt, base);
	if(s == nil)
		sysfatal("smprint: %r");

	if((fd = create(s, OWRITE, perm)) < 0)
		sysfatal("create %s: %r", s);
	(*fn)(fd);
	close(fd);
	free(s);
}

void
main(int argc, char **argv)
{
	fmtinstall('B', mpfmt);

	ARGBEGIN{
	default:
		usage();
	}ARGEND

	switch(argc){
	default:
		usage();
	case 1:
		base = argv[0];
	case 0:
		break;
	}

	do
		key = rsagen(1024, 6, 0);
	while(mpsignif(key->pub.n) != 1024);

	writefile("%s.secret", 0600, printsecret);
	writefile("%s.secret.factotum", 0600, printsecretfactotum);
	writefile("%s.public", 0666, printpublic);
//	writefile("%s.public10", 0666, printpublic10);
}
