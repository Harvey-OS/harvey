#include <u.h>
#include <libc.h>
#include <auth.h>
#include "../boot/boot.h"

#define DEFSYS "bootes"
typedef struct Net	Net;
typedef struct Flavor	Flavor;

int	printcol;

char	cputype[NAMELEN];
char	terminal[NAMELEN];
char	sys[2*NAMELEN];
char	username[NAMELEN];
char	bootfile[3*NAMELEN];
char	conffile[NAMELEN];

int mflag;
int fflag;
int kflag;

static void	swapproc(void);
static Method	*rootserver(char*);

static int
rconv(void *o, Fconv *fp)
{
	char s[ERRLEN];

	USED(o);

	s[0] = 0;
	errstr(s);
	strconv(s, fp);
	return 0;
}

void
dosboot(int argc, char *argv[])
{
	int fd;
	Method *mp;
	char cmd[64];
	char flags[6];
	int islocal;
	char rootdir[3*NAMELEN];

	sleep(1000);

	fmtinstall('r', rconv);

	open("#c/cons", OREAD);
	open("#c/cons", OWRITE);
	open("#c/cons", OWRITE);
/*	print("argc=%d\n", argc);
	for(fd = 0; fd < argc; fd++)
		print("%s ", argv[fd]);
	print("\n");/**/

	ARGBEGIN{
	}ARGEND

	readfile("#e/cputype", cputype, sizeof(cputype));
	readfile("#e/terminal", terminal, sizeof(terminal));
	getconffile(conffile, terminal);

	/*
	 *  pick a method and initialize it
	 */
	mp = rootserver(argc ? *argv : 0);
	(*mp->config)(mp);
	islocal = strcmp(mp->name, "local") == 0;

	/* set host's owner (and uid of current process) */
	if(writefile("#c/hostowner", "none", strlen("none")) < 0)
		fatal("can't set user name");

	/*
	 *  connect to the root file system
	 */
	fd = (*mp->connect)();
	if(fd < 0)
		fatal("can't connect to file server");
	srvcreate("boot", fd);

	/*
	 *  create the name space
	 */
	if(mount(fd, "/", MAFTER|MCREATE, "") < 0)
		fatal("mount");

	/*
	 *  hack to let us have the logical root in a
	 *  subdirectory - useful when we're the 'second'
	 *  OS along with some other like DOS.
	 */
	readfile("#e/rootdir", rootdir, sizeof(rootdir));
	if(rootdir[0]) {
		if(bind(rootdir, "/", MREPL|MCREATE) >= 0)
			bind("#/", "/", MBEFORE);
	}
	close(fd);

	settime(islocal);
	swapproc();
	remove("#e/password");	/* just in case */

	sprint(cmd, "/%s/init", cputype);
	sprint(flags, "-%s%s", cpuflag ? "c" : "t", mflag ? "m" : "");
	execl(cmd, "init", flags, 0);
	fatal(cmd);
}

/*
 *  ask user from whence cometh the root file system
 */
Method*
rootserver(char *arg)
{
	char prompt[256];
	char reply[64];
	Method *mp;
	char *cp, *goodarg;
	int n, j;

	goodarg = 0;
	mp = method;
	n = sprint(prompt, "root is from (%s", mp->name);
	if(arg && strncmp(arg, mp->name, strlen(mp->name)) == 0)
		goodarg = arg;
	for(mp++; mp->name; mp++){
		n += sprint(prompt+n, ", %s", mp->name);
		if(arg && strncmp(arg, mp->name, strlen(mp->name)) == 0)
			goodarg = arg;
	}
	sprint(prompt+n, ")");

	if(goodarg)
		strcpy(reply, goodarg);
	else {
		strcpy(reply, method->name);
	}
	for(;;){
		if(goodarg == 0)
			outin(cpuflag, prompt, reply, sizeof(reply));
		cp = strchr(reply, '!');
		if(cp)
			j = cp - reply;
		else
			j = strlen(reply);
		for(mp = method; mp->name; mp++)
			if(strncmp(reply, mp->name, j) == 0){
				if(cp)
					strcpy(sys, cp+1);
				return mp;
			}
		if(mp->name == 0)
			continue;
	}
	return 0;		/* not reached */
}

static void
swapproc(void)
{
	int fd;

	fd = open("#c/swap", OWRITE);
	if(fd < 0){
		warning("opening #c/swap");
		return;
	}
	if(write(fd, "start", 5) <= 0)
		warning("starting swap kproc");
}
