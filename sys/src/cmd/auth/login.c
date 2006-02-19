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
			fprint(2, "login: couldn't set raw mode");
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
			fprint(2, "login: can't read cons");
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

/*
 *  become the authenticated user
 */
void
chuid(AuthInfo *ai)
{
	int rv, fd;

	/* change uid */
	fd = open("#¤/capuse", OWRITE);
	if(fd < 0)
		sysfatal("can't change uid: %r");
	rv = write(fd, ai->cap, strlen(ai->cap));
	close(fd);
	if(rv < 0)
		sysfatal("can't change uid: %r");
}

/*
 *  mount a factotum
 */
void
mountfactotum(char *srvname)
{
	int fd;

	/* mount it */
	fd = open(srvname, ORDWR);
	if(fd < 0)
		sysfatal("opening factotum: %r");
	mount(fd, -1, "/mnt", MBEFORE, "");
	close(fd);
}

/*
 *  start a new factotum and pass it the username and password
 */
void
startfactotum(char *user, char *password, char *srvname)
{
	int fd;

	strcpy(srvname, "/srv/factotum.XXXXXXXXXXX");
	mktemp(srvname);

	switch(fork()){
	case -1:
		sysfatal("can't start factotum: %r");
	case 0:
		execl("/boot/factotum", "loginfactotum", "-ns", srvname+5, nil);
		sysfatal("starting factotum: %r");
		break;
	}

	/* wait for agent to really be there */
	while(access(srvname, 0) < 0)
		sleep(250);

	/* mount it */
	mountfactotum(srvname);

	/* write in new key */
	fd = open("/mnt/factotum/ctl", ORDWR);
	if(fd < 0)
		sysfatal("opening factotum: %r");
	fprint(fd, "key proto=p9sk1 dom=cs.bell-labs.com user=%q !password=%q", user, password);
	close(fd);
}

void
main(int argc, char *argv[])
{
	char pass[ANAMELEN];
	char buf[2*ANAMELEN];
	char home[2*ANAMELEN];
	char srvname[2*ANAMELEN];
	char *user, *sysname, *tz, *cputype, *service;
	AuthInfo *ai;

	ARGBEGIN{
	}ARGEND;

	rfork(RFENVG|RFNAMEG);

	service = getenv("service");
	if(strcmp(service, "cpu") == 0)
		fprint(2, "login: warning: running on a cpu server!\n");
	if(argc != 1){
		fprint(2, "usage: login username\n");
		exits("usage");
	}
	user = argv[0];
	memset(pass, 0, sizeof(pass));
	readln("Password: ", pass, sizeof(pass), 1);

	/* authenticate */
	ai = auth_userpasswd(user, pass);
	if(ai == nil || ai->cap == nil)
		sysfatal("login incorrect");

	/* change uid */
	chuid(ai);

	/* start a new factotum and hand it a new key */
	startfactotum(user, pass, srvname);

	/* set up new namespace */
	newns(ai->cuid, nil);
	auth_freeAI(ai);

	/* remount the factotum */
	mountfactotum(srvname);

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
	execl("/bin/rc", "rc", "-li", nil);
	exits(0);
}
