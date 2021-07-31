#include <u.h>
#include <libc.h>
#include <auth.h>
#include <ctype.h>
#include "authsrv.h"


void	install(char*, char*, char*, long, int);
int	exists (char*, char*);
long	getexpiration(char *db, char *u);
Tm	getdate(char*);

void
usage(void)
{
	fprint(2, "usage: changeuser [-hpn] user\n");
	exits("usage");
}

void
main(int argc, char *argv[])
{
	char *u, key[DESKEYLEN], answer[32];
	int which, i, newkey, newbio;
	long t;
	Acctbio a;
	Fs *f;

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
	argv0 = "changeuser";

	if(argc != 1)
		usage();
	u = *argv;
	if(memchr(u, '\0', NAMELEN) == 0)
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
			getpass(key, 1);
		t = getexpiration(f->keys, u);
		install(f->keys, u, key, t, newkey);
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
	char buf[KEYDBBUF+NAMELEN+6];
	int fd;

	if(!exists(db, u)){
		sprint(buf, "%s/%s", db, u);
		fd = create(buf, OREAD, 0777|CHDIR);
		if(fd < 0)
			error("can't create user %s: %r", u);
		close(fd);
	}

	if(newkey){
		sprint(buf, "%s/%s/key", db, u);
		fd = open(buf, OWRITE);
		if(fd < 0 || write(fd, key, DESKEYLEN) != DESKEYLEN)
			error("can't set key: %r");
		close(fd);
	}

	if(t == 0)
		return;
	sprint(buf, "%s/%s/expire", db, u);
	fd = open(buf, OWRITE);
	if(fd < 0 || fprint(fd, "%d", t) < 0)
		error("can't write expiration time");
	close(fd);
}

int
exists(char *db, char *u)
{
	char buf[KEYDBBUF+NAMELEN+6];

	sprint(buf, "%s/%s/expire", db, u);
	if(access(buf, OREAD) < 0)
		return 0;
	return 1;
}

/*
 * get the date in the format yyyymmdd
 */
Tm
getdate(char *d)
{
	Tm date;
	int i;

	date.year = date.mon = date.mday = 0;
	date.hour = date.min = date.sec = 0;
	for(i = 0; i < 8; i++)
		if(!isdigit(d[i]))
			return date;
	date.year = (d[0]-'0')*1000 + (d[1]-'0')*100 + (d[2]-'0')*10 + d[3]-'0';
	date.year -= 1900;
	d += 4;
	date.mon = (d[0]-'0')*10 + d[1]-'0';
	d += 2;
	date.mday = (d[0]-'0')*10 + d[1]-'0';
	return date;
}

long
getexpiration(char *db, char *u)
{
	char buf[32];
	char prompt[128];
	char cdate[32];
	Tm date;
	ulong secs, now, fd;
	int n;

	/* read current expiration (if any) */
	sprint(buf, "%s/%s/expire", db, u);
	fd = open(buf, OREAD);
	buf[0] = 0;
	if(fd >= 0){
		n = read(fd, buf, sizeof(buf)-1);
		if(n > 0)
			buf[n-1] = 0;
		close(fd);
	}
	secs = 0;
	if(buf[0]){
		if(strncmp(buf, "never", 5)){
			secs = atoi(buf);
			memmove(&date, localtime(secs), sizeof(date));
			sprint(buf, "%4.4d%2.2d%2.2d", date.year+1900, date.mon, date.mday);
		} else
			buf[5] = 0;
	} else
		strcpy(buf, "never");
	sprint(prompt, "Expiration date (YYYYMMDD or never)[return = %s]: ", buf);

	now = time(0);
	for(;;){
		readln(prompt, cdate, sizeof cdate, 0);
		if(*cdate == 0)
			return secs;
		if(strcmp(cdate, "never") == 0)
			return 0;
		date = getdate(cdate);
		secs = tm2sec(date);
		if(secs > now && secs < now + 2*365*24*60*60)
			break;
		print("expiration time must fall between now and 2 years from now\n");
	}
	return secs;
}
