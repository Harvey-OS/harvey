#include <u.h>
#include <libc.h>
#include <auth.h>
#include "authsrv.h"

enum{
	Plan9		= 1,
	Securenet	= 2,
};

void	install(char*, char*, char*, int);
void	usage(void);

void
main(int argc, char *argv[])
{
	char *u, key[DESKEYLEN], answer[32];
	int which, host, i, chal;

	srand(getpid()*time(0));
	fmtinstall('K', keyconv);

	which = 0;
	host = 0;
	ARGBEGIN{
	case 'h':
		host = 1;
		break;
	case 'p':
		which |= Plan9;
		break;
	case 'n':
		which |= Securenet;
		break;
	default:
		usage();
	}ARGEND
	argv0 = "changeuser";

	if(argc != 1)
		usage();
	u = *argv;
	if(memchr(u, '\0', NAMELEN) == 0)
		error("bad user name");

	if(!which)
		which = Plan9;
	if(which & Plan9){
		getpass(key, 1);
		install(KEYDB, u, key, host);
		print("user %s changed for Plan 9\n", u);
		syslog(0, AUTHLOG, "user %s changed for plan 9", u);
	}
	if(which & Securenet){
		for(i=0; i<DESKEYLEN; i++)
			key[i] = nrand(256);
		install(NETKEYDB, u, key, 0);
		print("user %s: SecureNet key: %K\n", u, key);
		chal = lnrand(MAXNETCHAL);
		print("verify with challenge %lud response %s\n",
			chal, netdecimal(netresp(key, chal, answer)));
		print("user %s changed for SecureNet\n", u);
		syslog(0, AUTHLOG, "user %s changed for securenet", u);
	}
	exits(0);
}

void
install(char *db, char *u, char *key, int host)
{
	char buf[KEYDBBUF+NAMELEN+6];
	int fd;

	sprint(buf, "%s/%s/key", db, u);
	fd = open(buf, OWRITE);
	if(fd < 0 || write(fd, key, DESKEYLEN) != DESKEYLEN)
		error("can't set key");
	close(fd);
	if(host){
		sprint(buf, "%s/%s/ishost", db, u);
		fd = create(buf, OREAD, 0777);
		if(fd < 0)
			error("can't make %s a host: %r", u);
	}
}

void
usage(void)
{
	fprint(2, "usage: changeuser [-hpn] user\n");
	exits("usage");
}
