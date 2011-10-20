#include <u.h>
#include <libc.h>
#include <authsrv.h>
#include <ctype.h>
#include <bio.h>
#include "authcmdlib.h"

void	install(char*, char*, char*, long, int);
int	exists (char*, char*);

void
usage(void)
{
	fprint(2, "usage: changeuser [-pn] user\n");
	exits("usage");
}

void
main(int argc, char *argv[])
{
	char *u, key[DESKEYLEN], answer[32], p9pass[32];
	int which, i, newkey, newbio, dosecret;
	long t;
	Acctbio a;
	Fs *f;

	srand(getpid()*time(0));
	fmtinstall('K', keyfmt);

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
	argv0 = "changeuser";

	if(argc != 1)
		usage();
	u = *argv;
	if(memchr(u, '\0', ANAMELEN) == 0)
		error("bad user name");

	if(!which)
		which = Plan9;

	newbio = 0;
	t = 0;
	a.user = 0;
	if(which & Plan9){
		f = &fs[Plan9];
		newkey = 1;
		if(exists(f->keys, u)){
			readln("assign new password? [y/n]: ", answer, sizeof answer, 0);
			if(answer[0] != 'y' && answer[0] != 'Y')
				newkey = 0;
		}
		if(newkey)
			getpass(key, p9pass, 1, 1);
		dosecret = getsecret(newkey, p9pass);
		t = getexpiration(f->keys, u);
		install(f->keys, u, key, t, newkey);
		if(dosecret && setsecret(KEYDB, u, p9pass) == 0)
			error("error writing Inferno/pop secret");
		newbio = querybio(f->who, u, &a);
		if(newbio)
			wrbio(f->who, &a);
		print("user %s installed for Plan 9\n", u);
		syslog(0, AUTHLOG, "user %s installed for plan 9", u);
	}
	if(which & Securenet){
		f = &fs[Securenet];
		newkey = 1;
		if(exists(f->keys, u)){
			readln("assign new key? [y/n]: ", answer, sizeof answer, 0);
			if(answer[0] != 'y' && answer[0] != 'Y')
				newkey = 0;
		}
		if(newkey)
			for(i=0; i<DESKEYLEN; i++)
				key[i] = nrand(256);
		if(a.user == 0){
			t = getexpiration(f->keys, u);
			newbio = querybio(f->who, u, &a);
		}
		install(f->keys, u, key, t, newkey);
		if(newbio)
			wrbio(f->who, &a);
		findkey(f->keys, u, key);
		print("user %s: SecureNet key: %K\n", u, key);
		checksum(key, answer);
		print("verify with checksum %s\n", answer);
		print("user %s installed for SecureNet\n", u);
		syslog(0, AUTHLOG, "user %s installed for securenet", u);
	}
	exits(0);
}

void
install(char *db, char *u, char *key, long t, int newkey)
{
	char buf[KEYDBBUF+ANAMELEN+20];
	int fd;

	if(!exists(db, u)){
		snprint(buf, sizeof buf, "%s/%s", db, u);
		fd = create(buf, OREAD, 0777|DMDIR);
		if(fd < 0)
			error("can't create user %s: %r", u);
		close(fd);
	}

	if(newkey){
		snprint(buf, sizeof buf, "%s/%s/key", db, u);
		fd = open(buf, OWRITE);
		if(fd < 0 || write(fd, key, DESKEYLEN) != DESKEYLEN)
			error("can't set key: %r");
		close(fd);
	}

	if(t == -1)
		return;
	snprint(buf, sizeof buf, "%s/%s/expire", db, u);
	fd = open(buf, OWRITE);
	if(fd < 0 || fprint(fd, "%ld", t) < 0)
		error("can't write expiration time");
	close(fd);
}

int
exists(char *db, char *u)
{
	char buf[KEYDBBUF+ANAMELEN+6];

	snprint(buf, sizeof buf, "%s/%s/expire", db, u);
	if(access(buf, 0) < 0)
		return 0;
	return 1;
}
