#include <u.h>
#include <libc.h>
#include <auth.h>
#include "authsrv.h"

enum{
	Plan9		= 1,
	Securenet	= 2,
};

void	install(char*, char*, char*, int);
void	check(char*, char*, char*);
void	usage(void);

void
main(int argc, char *argv[])
{
	char *u, key[DESKEYLEN], answer[32];
	long chal;
	int which, host, i;

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
	argv0 = "adduser";

	if(argc != 1)
		usage();
	u = *argv;
	if(memchr(u, '\0', NAMELEN) == 0)
		error("bad user name");

	if(!which)
		which = Plan9;
	if(which & Plan9)
		check(KEYDB, u, "Plan 9");
	if(which & Securenet)
		check(NETKEYDB, u, "SecureNet");
	if(which & Plan9){
		getpass(key, 1);
		install(KEYDB, u, key, host);
		print("user %s installed for Plan 9\n", u);
		syslog(0, AUTHLOG, "user %s installed for plan 9", u);
	}
	if(which & Securenet){
		for(i=0; i<DESKEYLEN; i++)
			key[i] = nrand(256);
		print("user %s: SecureNet key: %K\n", u, key);
		install(NETKEYDB, u, key, 0);
		print("user %s: SecureNet key: %K\n", u, key);
		chal = lnrand(MAXNETCHAL);
		print("verify with challenge %lud response %s\n",
			chal, netdecimal(netresp(key, chal, answer)));
		print("user %s installed for SecureNet\n", u);
		syslog(0, AUTHLOG, "user %s installed for securenet", u);
	}
	exits(0);
}

void
check(char *db, char *u, char *where)
{
	char buf[KEYDBBUF+NAMELEN+6];
	char dir[DIRLEN];

	sprint(buf, "%s/%s", db, u);
	if(stat(buf, dir) >= 0)
		error("user %s already exists for %s", u, where);
}

void
install(char *db, char *u, char *key, int host)
{
	char buf[KEYDBBUF+2*NAMELEN+6];
	int fd;

	sprint(buf, "%s/%s", db, u);
	fd = create(buf, OREAD, 0777|CHDIR);
	if(fd < 0)
		error("can't create user %s: %r", u);
	close(fd);
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
	fprint(2, "usage: adduser [-hpn] user\n");
	exits("usage");
}
