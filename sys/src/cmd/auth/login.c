#include <u.h>
#include <libc.h>
#include <auth.h>
#include <authsrv.h>

void
readln(char *prompt, char *line, int len, int raw)
{
	char *p;
	int fdin, fdout, ctl, n, nr;

	fdin = open("/dev/cons", OREAD);
	fdout = open("/dev/cons", OWRITE);
	fprint(fdout, "%s", prompt);
	if(raw){
		ctl = open("/dev/consctl", OWRITE);
		if(ctl < 0){
			fprint(2, "couldn't set raw mode");
			exits("readln");
		}
		write(ctl, "rawon", 5);
	} else
		ctl = -1;
	nr = 0;
	p = line;
	for(;;){
		n = read(fdin, p, 1);
		if(n < 0){
			close(ctl);
			close(fdin);
			close(fdout);
			fprint(2, "can't read cons");
			exits("readln");
		}
		if(*p == 0x7f)
			exits(0);
		if(n == 0 || *p == '\n' || *p == '\r'){
			*p = '\0';
			if(raw){
				write(ctl, "rawoff", 6);
				write(fdout, "\n", 1);
			}
			close(ctl);
			close(fdin);
			close(fdout);
			return;
		}
		if(*p == '\b'){
			if(nr > 0){
				nr--;
				p--;
			}
		}else{
			nr++;
			p++;
		}
		if(nr == len){
			fprint(fdout, "line too long; try again\n");
			nr = 0;
			p = line;
		}
	}
}

void
setenv(char *var, char *val)
{
	int fd;

	fd = create(var, OWRITE, 0644);
	if(fd < 0)
		print("init: can't open %s\n", var);
	else{
		fprint(fd, val);
		close(fd);
	}
}

void
main(int argc, char *argv[])
{
	char pass[ANAMELEN];
	char buf[2*ANAMELEN];
	char home[2*ANAMELEN];
	char *user, *sysname, *tz, *cputype, *service;

	service = getenv("service");
	if(strcmp(service, "cpu") == 0){
		fprint(2, "not from a cpu server!\n");
		exits("fail");
	}
	if(argc != 2){
		fprint(2, "usage: login username\n");
		exits("usage");
	}
	user = argv[1];
	memset(pass, 0, sizeof(pass));
	readln("Password: ", pass, sizeof(pass), 1);
	if(login(argv[1], pass, nil) < 0){
		fprint(2, "login failed: %r\n");
		exits("fail");
	}

	/* set up a new environment */
	cputype = getenv("cputype");
	sysname = getenv("sysname");
	tz = getenv("timezone");
	rfork(RFCENVG);
	setenv("#e/service", "con");
	setenv("#e/user", user);
	snprint(home, sizeof(home), "/usr/%s", user);
	setenv("#e/home", home);
	setenv("#e/cputype", cputype);
	setenv("#e/objtype", cputype);
	if(sysname != nil)
		setenv("#e/sysname", sysname);
	if(tz != nil)
		setenv("#e/timezone", tz);

	/* go to new home directory */
	snprint(buf, sizeof(buf), "/usr/%s", user);
	if(chdir(buf) < 0)
		chdir("/");

	/* read profile and start interactive rc */
	execl("/bin/rc", "rc", "-li", 0);
	exits(0);
}
