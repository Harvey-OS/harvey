#include <u.h>
#include <libc.h>
#include <auth.h>
#include "authsrv.h"

void	install(char*, char*, char*, long);
void	check(char*, char*, char*);
void	getbio(int, char*);
void	usage(void);

void
main(int argc, char *argv[])
{
	char *u, key[DESKEYLEN], answer[32];
	int which, i;
	long t = -1;

	srand(getpid()*time(0));
	fmtinstall('K', keyconv);

	which = 0;
	ARGBEGIN{
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
		t = getexpiration();
		install(KEYDB, u, key, t);
		getbio(which, u);
		print("user %s installed for Plan 9\n", u);
		syslog(0, AUTHLOG, "user %s installed for plan 9", u);
	}
	if(which & Securenet){
		for(i=0; i<DESKEYLEN; i++)
			key[i] = nrand(256);
		if(t < 0){
			install(NETKEYDB, u, key, getexpiration());
			getbio(which, u);
		} else
			install(NETKEYDB, u, key, t);
		print("user %s: SecureNet key: %K\n", u, key);
		checksum(key, answer);
		print("verify with checksum %s\n", answer);
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
install(char *db, char *u, char *key, long t)
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
	if(t == 0)
		return;
	sprint(buf, "%s/%s/expire", db, u);
	fd = open(buf, OWRITE);
	if(fd < 0 || fprint(fd, "%d", t) < 0)
		error("can't write expiration time");
	close(fd);
}

void
usage(void)
{
	fprint(2, "usage: adduser [-hpn] user\n");
	exits("usage");
}

#define TABLEN 8

void
getbio(int which, char *user)
{
	char uemail[256];
	char semail[256];
	char name[256];
	char dept[256];
	char uinfo[512];
	int t1, t2;
	char t1s[16], t2s[16];
	int fd;

	readln("User's email address: ", uemail, sizeof uemail, 0);
	readln("User's full name: ", name, sizeof name, 0);
	readln("Sponsor's email address (return if none): ", semail, sizeof semail, 0);
	readln("Department #: ", dept, sizeof dept, 0);

	if(which&Plan9)
		fd = open("/adm/keys.who", OWRITE);
	else
		fd = open("/adm/netkeys.who", OWRITE);
	if(fd < 0)
		error("can't open /adm/*keys.who");
	if(seek(fd, 0, 2) < 0)
		error("can't seek /adm/*keys.who");

	sprint(uinfo, "%s %s <%s>", name, dept, uemail);
	t1 = 1+(16-strlen(user)+7)/TABLEN;
	memset(t1s, '\t', t1);
	t1s[t1] = 0;
	t2 = 1+(48-strlen(uinfo)+7)/TABLEN;
	memset(t2s, '\t', t2);
	t2s[t2] = 0;
	fprint(fd, "%s%s%s%s<%s>\n", user, t1s, uinfo, t2s, semail);

	close(fd);
}
