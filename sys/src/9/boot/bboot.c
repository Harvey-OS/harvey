#include <u.h>
#include <libc.h>
#include <bio.h>
#include <mach.h>
#include "../boot/boot.h"

typedef struct Net	Net;
typedef struct Flavor	Flavor;

int	printcol;

char	cputype[NAMELEN];
char	terminal[NAMELEN];
char	sys[2*NAMELEN];
char	username[NAMELEN];
char	conffile[2*NAMELEN];
char	sysname[2*NAMELEN];
char	buf[8*1024];
char	bootfile[3*NAMELEN];
char	conffile[NAMELEN];

int mflag;
int fflag;
int kflag;

static int	readconf(int);
static void	readkernel(int);
static int	readseg(int, int, long, long, int);
static Method	*rootserver(char*);

void
bboot(int argc, char *argv[])
{
	int fd;
	Method *mp;
	int islocal;

	sleep(1000);

	open("#c/cons", OREAD);
	open("#c/cons", OWRITE);
	open("#c/cons", OWRITE);

/*	for(fd = 0; fd < argc; fd++)
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
	readfile("#c/sysname", sysname, sizeof(sysname));
	if(argc > 1)
		strcpy(bootfile, argv[1]);
	else
		sprint(bootfile, "/%s/9%s", cputype, conffile);

	/*
	 *  pick a method and initialize it
	 */
	mp = rootserver(argc ? *argv : 0);
	(*mp->config)(mp);
	islocal = strcmp(mp->name, "local") == 0;

	/*
	 *  set user to none
	 */
	fd = open("#c/user", OWRITE|OTRUNC);
	if(fd >= 0){
		if(write(fd, "none", 4) < 0)
			warning("write user name");
		close(fd);
	}else
		warning("open #c/user");

	/*
	 *  connect to the root file system
	 */
	fd = (*mp->connect)();
	if(fd < 0)
		fatal("can't connect to file server");
	if(!islocal){
		nop(fd);
		session(fd);
		if(cfs)
			fd = (*cfs)(fd);
	}

	/*
	 *  create the name space, mount the root fs
	 */
	if(bind("/", "/", MREPL) < 0)
		fatal("bind");
	if(mount(fd, "/", MAFTER|MCREATE, "", "") < 0)
		fatal("mount");
	close(fd);

	/*
	 *  open the configuration file and read it
	 *  into the kernel
	 */
	sprint(conffile, "/%s/conf/%s", cputype, sysname);
	print("%s...", conffile);
	fd = open(conffile, OREAD);
	if(fd >= 0){
		if(readconf(fd) < 0)
			warning("readconf");
		close(fd);
	}

	/*
	 *  read in real kernel
	 */
	print("%s...", bootfile);
	while((fd = open(bootfile, OREAD)) < 0)
		outin("bootfile", bootfile, sizeof(bootfile));
	readkernel(fd);
	fatal("couldn't read kernel");
}

/*
 *  ask user from whence cometh the root file system
 */
static Method*
rootserver(char *arg)
{
	char prompt[256];
	char reply[64];
	Method *mp;
	char *cp;
	int n;

	mp = method;
	n = sprint(prompt, "root is from (%s", mp->name);
	for(mp++; mp->name; mp++)
		n += sprint(prompt+n, ", %s", mp->name);
	sprint(prompt+n, ")");

	if(arg)
		strcpy(reply, arg);
	else
		strcpy(reply, method->name);
	for(;;){
		if(mflag)
			outin(prompt, reply, sizeof(reply));
		for(mp = method; mp->name; mp++)
			if(*reply == *mp->name){
				cp = strchr(reply, '!');
				if(cp)
					strcpy(sys, cp+1);
				return mp;
			}
		if(mp->name == 0)
			continue;
	}
	return 0;	/* not reached */
}


/*
 *  read the configuration
 */
static int
readconf(int fd)
{
	int bfd;
	int n;
	long x;

	/*
 	 *  read the config file
	 */
	n = read(fd, buf, sizeof(buf)-1);
	if(n <= 0)
		return -1;
	buf[n] = 0;

	/*
	 *  write into 4 meg - 4k
	 */
	bfd = open("#B/mem", OWRITE);
	if(bfd < 0)
		fatal("can't open #B/mem");
	x = 0x80000000 | 4*1024*1024 - 4*1024;
	if(seek(bfd, x, 0) != x){
		close(bfd);
		return -1;
	}
	if(write(bfd, buf, n+1) != n+1){
		close(bfd);
		return -1;
	}

	close(bfd);
	return 0;
}

/*
 *  read the kernel into memory and jump to it
 */
static void
readkernel(int fd)
{
	int bfd;
	Fhdr f;

	bfd = open("#B/mem", OWRITE);
	if(bfd < 0)
		fatal("can't open #B/mem");

	if(crackhdr(fd, &f) == 0)
		fatal("not a header I understand");

	print("\n%d", f.txtsz);
	if(readseg(fd, bfd, f.txtoff, f.txtaddr, f.txtsz)<0)
		fatal("can't read boot file");
	print("+%d", f.datsz);
	if(readseg(fd, bfd, f.datoff, f.dataddr, f.datsz)<0)
		fatal("can't read boot file");
	print("+%d", f.bsssz);

	close(bfd);
	bfd = open("#B/boot", OWRITE);
	if(bfd < 0)
		fatal("can't open #B/boot");
	
	print(" entry: 0x%ux\n", f.entry);
	sleep(1000);
	if(write(bfd, &f.entry, sizeof f.entry) != sizeof f.entry)
		fatal("can't start kernel");
}

/*
 *  read a segment into memory
 */
static int
readseg(int in, int out, long inoff, long outoff, int len)
{
	long n;

	if(seek(in, inoff, 0) < 0){
		warning("seeking bootfile");
		return -1;
	}
	if(seek(out, outoff, 0) != outoff){
		warning("seeking #B/mem");
		return -1;
	}
	for(; len > 0; len -= n){
		if((n = read(in, buf, sizeof buf)) <= 0){
			warning("reading bootfile");
			return -1;
		}
		if(write(out, buf, n) != n){
			warning("writing #B/mem");
			return -1;
		}
	}
	return 0;
}

