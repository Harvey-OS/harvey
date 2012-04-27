#include <u.h>
#include <libc.h>
#include <mp.h>
#include <libsec.h>
#include "SConn.h"
#include "secstore.h"

int verbose;

static void userinput(char *, int);

static void
ensure_exists(char *f, ulong perm)
{
	int fd;

	if(access(f, AEXIST) >= 0)
		return;
	if(verbose)
		fprint(2,"first time setup for secstore: create %s %lo\n", f, perm);
	fd = create(f, OREAD, perm);
	if(fd < 0)
		sysfatal("unable to create %s: %r", f);
	close(fd);
}


void
main(int argc, char **argv)
{
	int isnew;
	char *id, buf[Maxmsg], home[Maxmsg], prompt[100], *hexHi;
	char *pass, *passck;
	long expsecs;
	mpint *H = mpnew(0), *Hi = mpnew(0);
	PW *pw;
	Tm *tm;

	ARGBEGIN{
	case 'v':
		verbose++;
		break;
	}ARGEND;
	if(argc!=1){
		fprint(2, "usage: secuser [-v] <user>\n");
		exits("usage");
	}

	ensure_exists(SECSTORE_DIR, DMDIR|0755L);
	snprint(home, sizeof(home), "%s/who", SECSTORE_DIR);
	ensure_exists(home, DMDIR|0755L);
	snprint(home, sizeof(home), "%s/store", SECSTORE_DIR);
	ensure_exists(home, DMDIR|0700L);

	id = argv[0];
	if(verbose)
		fprint(2,"secuser %s\n", id);
	if((pw = getPW(id,1)) == nil){
		isnew = 1;
		print("new account (because %s/%s %r)\n", SECSTORE_DIR, id);
		pw = emalloc(sizeof(*pw));
		pw->id = estrdup(id);
		snprint(home, sizeof(home), "%s/store/%s", SECSTORE_DIR, id);
		if(access(home, AEXIST) == 0)
			sysfatal("new user, but directory %s already exists",
				home);
	}else{
		isnew = 0;
	}

	/* get main password for id */
	for(;;){
		if(isnew)
			snprint(prompt, sizeof(prompt), "%s password: ", id);
		else
			snprint(prompt, sizeof(prompt), "%s password [default = don't change]: ", id);
		pass = getpassm(prompt);
		if(pass == nil)
			sysfatal("getpassm failed");
		if(verbose)
			print("%ld characters\n", strlen(pass));
		if(pass[0] == '\0' && isnew == 0)
			break;
		if(strlen(pass) >= 7)
			break;
		print("password must be at least 7 characters\n");
	}

	if(pass[0] != '\0'){
		snprint(prompt, sizeof(prompt), "retype password: ");
		if(verbose)
			print("confirming...\n");
		passck = getpassm(prompt);
		if(passck == nil)
			sysfatal("getpassm failed");
		if(strcmp(pass, passck) != 0)
			sysfatal("passwords didn't match");
		memset(passck, 0, strlen(passck));
		free(passck);
		hexHi = PAK_Hi(id, pass, H, Hi);
		memset(pass, 0, strlen(pass));
		free(pass);
		free(hexHi);
		mpfree(H);
		pw->Hi = Hi;
	}

	/* get expiration time (midnight of date specified) */
	if(isnew)
		expsecs = time(0) + 365*24*60*60;
	else
		expsecs = pw->expire;

	for(;;){
		tm = localtime(expsecs);
		print("expires [DDMMYYYY, default = %2.2d%2.2d%4.4d]: ",
				tm->mday, tm->mon+1, tm->year+1900);
		userinput(buf, sizeof(buf));
		if(strlen(buf) == 0)
			break;
		if(strlen(buf) != 8){
			print("!bad date format: %s\n", buf);
			continue;
		}
		tm->mday = (buf[0]-'0')*10 + (buf[1]-'0');
		if(tm->mday > 31 || tm->mday < 1){
			print("!bad day of month: %d\n", tm->mday);
			continue;
		}
		tm->mon = (buf[2]-'0')*10 + (buf[3]-'0') - 1;
		if(tm->mon > 11 || tm->mon < 0){
			print("!bad month: %d\n", tm->mon + 1);
			continue;
		}
		tm->year = atoi(buf+4) - 1900;
		if(tm->year < 70){
			print("!bad year: %d\n", tm->year + 1900);
			continue;
		}
		tm->sec = 59;
		tm->min = 59;
		tm->hour = 23;
		tm->yday = 0;
		expsecs = tm2sec(tm);
		break;
	}
	pw->expire = expsecs;

	/* failed logins */
	if(pw->failed != 0 )
		print("clearing %d failed login attempts\n", pw->failed);
	pw->failed = 0;

	/* status bits */
	if(isnew)
		pw->status = Enabled;
	for(;;){
		print("Enabled or Disabled [default %s]: ",
			(pw->status & Enabled) ? "Enabled" : "Disabled" );
		userinput(buf, sizeof(buf));
		if(strlen(buf) == 0)
			break;
		if(buf[0]=='E' || buf[0]=='e'){
			pw->status |= Enabled;
			break;
		}
		if(buf[0]=='D' || buf[0]=='d'){
			pw->status = pw->status & ~Enabled;
			break;
		}
	}
	for(;;){
		print("require STA? [default %s]: ",
			(pw->status & STA) ? "yes" : "no" );
		userinput(buf, sizeof(buf));
		if(strlen(buf) == 0)
			break;
		if(buf[0]=='Y' || buf[0]=='y'){
			pw->status |= STA;
			break;
		}
		if(buf[0]=='N' || buf[0]=='n'){
			pw->status = pw->status & ~STA;
			break;
		}
	}

	/* free form field */
	if(isnew)
		pw->other = nil;
	print("comments [default = %s]: ", (pw->other == nil) ? "" : pw->other);
	userinput(buf, 72);  /* 72 comes from password.h */
	if(buf[0])
		if((pw->other = strdup(buf)) == nil)
			sysfatal("strdup");

	syslog(0, LOG, "CHANGELOGIN for '%s'", pw->id);
	if(putPW(pw) < 0)
		sysfatal("can't write password file: %r");
	else{
		print("change written\n");
		if(isnew && create(home, OREAD, DMDIR | 0775L) < 0)
			sysfatal("unable to create %s: %r", home);
	}

	exits("");
}


static void
userinput(char *buf, int blen)
{
	int n;

	for(;;){
		n = read(0, buf, blen);
		if(n<=0)
			exits("read error");
		if(buf[n-1]=='\n'){
			buf[n-1] = '\0';
			return;
		}
		buf += n;  blen -= n;
		if(blen<=0)
			exits("input too large");
	}
}
