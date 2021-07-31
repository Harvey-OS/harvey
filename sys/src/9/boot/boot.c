#include <u.h>
#include <libc.h>
#include "../boot/boot.h"

#define DEFSYS "bootes"
typedef struct Net	Net;
typedef struct Flavor	Flavor;

int	printcol;

char	cputype[NAMELEN];
char	terminal[NAMELEN];
char	sys[2*NAMELEN];
char	username[NAMELEN];
char	*sauth;
char	bootfile[3*NAMELEN];
char	conffile[NAMELEN];

int mflag;
int fflag;
int kflag;
int aflag;
int pflag;

static void	swapproc(void);
static Method	*rootserver(char*);

void
boot(int argc, char *argv[])
{
	int fd;
	Method *mp;
	char cmd[64];
	char flags[6];
	int islocal, ishybrid;

	sleep(1000);

	open("#c/cons", OREAD);
	open("#c/cons", OWRITE);
	open("#c/cons", OWRITE);
/*	print("argc=%d\n", argc);
	for(fd = 0; fd < argc; fd++)
		print("%s ", argv[fd]);
	print("\n");/**/

	if(argc <= 1)
		pflag = 1;

	ARGBEGIN{
	case 'a':
		aflag = 1;
		break;
	case 'u':
		strcpy(username, ARGF());
		break;
	case 'k':
		kflag = 1;
		break;
	case 'm':
		pflag = 1;
		mflag = 1;
		break;
	case 'p':
		pflag = 1;
		break;
	case 'f':
		fflag = 1;
		break;
	}ARGEND

	readfile("#e/cputype", cputype, sizeof(cputype));
	readfile("#e/terminal", terminal, sizeof(cputype));
	getconffile(conffile, terminal);

	/*
	 *  pick a method and initialize it
	 */
	mp = rootserver(argc ? *argv : 0);
	(*mp->config)(mp);
	islocal = strcmp(mp->name, "local") == 0;
	ishybrid = strcmp(mp->name, "hybrid") == 0;

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
		nop(fd);
		session(fd);
		if(cfs)
			fd = (*cfs)(fd);
	}
	srvcreate("boot", fd);

	/*
	 *  create the name space, mount the root fs
	 */
	if(bind("/", "/", MREPL) < 0)
		fatal("bind");
	sauth = "";
	if(mount(fd, "/", MAFTER|MCREATE, "", sauth) < 0){
		sauth = "any";
		if(mount(fd, "/", MAFTER|MCREATE, "", sauth) < 0)
			fatal("mount");
	}
	close(fd);
	if(cpuflag == 0)
		newkernel();

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
			mount(fd, "/n/kfs", MAFTER|MCREATE, "", "") ;
			close(fd);
			break;
		}
	}

	settime(islocal);
	swapproc();

	sprint(cmd, "/%s/init", cputype);
	sprint(flags, "-%s%s%s", cpuflag ? "c" : "t", mflag ? "m" : "", aflag ? "a" : "");
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
	char *cp;
	int n, j;
	int notfirst;

	mp = method;
	n = sprint(prompt, "root is from (%s", mp->name);
	for(mp++; mp->name; mp++)
		n += sprint(prompt+n, ", %s", mp->name);
	sprint(prompt+n, ")");

	if(arg)
		strcpy(reply, arg);
	else
		strcpy(reply, method->name);
	for(notfirst = 0;; notfirst = 1){
		if(pflag || notfirst)
			outin(prompt, reply, sizeof(reply));
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
