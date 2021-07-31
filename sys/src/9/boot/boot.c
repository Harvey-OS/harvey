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
int afd = -1;

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
boot(int argc, char *argv[])
{
	int fd;
	Method *mp;
	char cmd[64];
	char flags[6];
	int islocal, ishybrid;
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
	case 'u':
		strcpy(username, ARGF());
		break;
	case 'k':
		kflag = 1;
		break;
	case 'm':
		mflag = 1;
		break;
	case 'f':
		fflag = 1;
		break;
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
	ishybrid = (mp->name[0] == 'h' || mp->name[0] == 'H') &&
			strcmp(&mp->name[1], "ybrid") == 0;

	/*
	 *  get/set key or password
	 */
	(*pword)(islocal, mp);

	/*
	 *  connect to the root file system
	 */
	fd = (*mp->connect)();
	if(fd < 0)
		fatal("can't connect to file server");
	if(!islocal && !ishybrid){
		if(cfs)
			fd = (*cfs)(fd);
		doauthenticate(fd, mp);
	}
	srvcreate("boot", fd);

	/*
	 *  create the name space
	 */
	if(bind("/", "/", MREPL) < 0)
		fatal("bind");
	if(mount(fd, "/", MAFTER|MCREATE, "") < 0)
		fatal("mount");
	close(fd);

	/*
	 *  hack to let us have the logical root in a
	 *  subdirectory - useful when we're the 'second'
	 *  OS along with some other like DOS.
	 */
	readfile("#e/rootdir", rootdir, sizeof(rootdir));
	if(rootdir[0])
		if(bind(rootdir, "/", MREPL|MCREATE) >= 0)
			bind("#/", "/", MBEFORE);

	/*
	 *  if a local file server exists and it's not
	 *  running, start it and mount it onto /n/kfs
	 */
	if(access("#s/kfs", 0) < 0){
		for(mp = method; mp->name; mp++){
			if(strcmp(mp->name, "local") != 0)
				continue;
			(*mp->config)(mp);
			fd = (*mp->connect)();
			if(fd < 0)
				break;
			mount(fd, "/n/kfs", MAFTER|MCREATE, "") ;
			close(fd);
			break;
		}
	}

	settime(islocal);
	close(afd);
	swapproc();
	remove("#e/password");

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
