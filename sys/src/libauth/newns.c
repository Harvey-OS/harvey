#include <u.h>
#include <libc.h>
#include <bio.h>
#include <auth.h>
#include <authsrv.h>
#include "authlocal.h"

enum
{
	NARG	= 15,		/* max number of arguments */
	MAXARG	= 10*ANAMELEN,	/* max length of an argument */
};

static int	setenv(char*, char*);
static char	*expandarg(char*, char*);
static int	splitargs(char*, char*[], char*, int);
static void	nsop(int, char*[], AuthRpc*);
static int	callexport(char*, char*);
static int	catch(void*, char*);

static int
buildns(int newns, char *user, char *file)
{
	Biobuf *spec;
	char *cmd, *argv[NARG], argbuf[MAXARG*NARG];
	char home[4*ANAMELEN];
	int argc;
	int afd;
	AuthRpc *rpc;

	rpc = nil;
	/* try for factotum now because later is impossible */
	afd = open("/mnt/factotum/rpc", ORDWR);
	if(afd >= 0){
		rpc = auth_allocrpc(afd);
		if(rpc == nil){
			close(afd);
			afd = -1;
		}
	}
	if(newns && !file){
		/* first look for a namespace file for this system */
		snprint(home, sizeof(home), "/lib/namespace.%s", sysname());	
		if(access(home, 4) == 0)
			file = home;
		else
			file = "/lib/namespace";
	}
	spec = Bopen(file, OREAD);
	if(spec == 0){
		werrstr("can't open %s: %r", file);
		close(afd);
		auth_freerpc(rpc);
		return -1;
	}
	if(newns){
		rfork(RFENVG|RFCNAMEG);
		setenv("user", user);
		snprint(home, 2*ANAMELEN, "/usr/%s", user);
		setenv("home", home);
	}
	atnotify(catch, 1);
	while(cmd = Brdline(spec, '\n')){
		cmd[Blinelen(spec)-1] = '\0';
		while(*cmd==' ' || *cmd=='\t')
			cmd++;
		if(*cmd == '#')
			continue;
		argc = splitargs(cmd, argv, argbuf, NARG);
		if(argc)
			nsop(argc, argv, rpc);
	}
	atnotify(catch, 0);
	Bterm(spec);
	if(rpc){
		close(rpc->afd);
		auth_freerpc(rpc);
	}
	return 0;
}

int
newns(char *user, char *file)
{
	return buildns(1, user, file);
}

int
addns(char *user, char *file)
{
	return buildns(0, user, file);
}

static int
famount(int fd, AuthRpc *rpc, char *mntpt, int flags, char *aname)
{
	int afd;
	AuthInfo *ai;

	afd = fauth(fd, aname);
	if(afd >= 0){
		ai = fauth_proxy(afd, rpc, amount_getkey, "proto=p9any role=client");
		if(ai != nil)
			auth_freeAI(ai);
	}
	return mount(fd, afd, mntpt, flags, aname);
}

static void
nsop(int argc, char *argv[], AuthRpc *rpc)
{
	char *argv0;
	ulong flags;
	int fd;

	flags = 0;
	argv0 = 0;
	ARGBEGIN{
	case 'a':
		flags |= MAFTER;
		break;
	case 'b':
		flags |= MBEFORE;
		break;
	case 'c':
		flags |= MCREATE;
		break;
	case 'C':
		flags |= MCACHE;
		break;
	}ARGEND

	if(!(flags & (MAFTER|MBEFORE)))
		flags |= MREPL;

	if(strcmp(argv0, "bind") == 0 && argc == 2)
		bind(argv[0], argv[1], flags);
	else if(strcmp(argv0, "unmount") == 0){
		if(argc == 1)
			unmount(nil, argv[0]);
		else if(argc == 2)
			unmount(argv[0], argv[1]);
	}else if(strcmp(argv0, "mount") == 0){
		fd = open(argv[0], ORDWR);
		if(argc == 2)
			famount(fd, rpc, argv[1], flags, "");
		else if(argc == 3)
			famount(fd, rpc, argv[1], flags, argv[2]);
		close(fd);
	} else if(strcmp(argv0, "import") == 0){
		fd = callexport(argv[0], argv[1]);
		if(argc == 2)
			famount(fd, rpc, argv[1], flags, "");
		else if(argc == 3)
			famount(fd, rpc, argv[2], flags, "");
		close(fd);
	} else if(strcmp(argv0, "cd") == 0 && argc == 1)
		chdir(argv[0]);
}

static char *wocp = "sys: write on closed pipe";

static int
catch(void *x, char *m)
{
	USED(x);
	return strncmp(m, wocp, strlen(wocp)) == 0;
}

static int
callexport(char *sys, char *tree)
{
	char *na, buf[3];
	int fd;
	AuthInfo *ai;

	na = netmkaddr(sys, 0, "exportfs");
	if((fd = dial(na, 0, 0, 0)) < 0)
		return -1;
	if((ai = auth_proxy(fd, auth_getkey, "proto=p9any role=client")) == nil
	|| write(fd, tree, strlen(tree)) < 0
	|| read(fd, buf, 3) != 2 || buf[0]!='O' || buf[1]!= 'K'){
		close(fd);
		auth_freeAI(ai);
		return -1;
	}
	auth_freeAI(ai);
	return fd;
}

static int
splitargs(char *p, char *argv[], char *argbuf, int maxargs)
{
	char *q;
	int i;

	i = 0;
	while(i < maxargs){
		while(*p==' ' || *p=='\t')
			p++;
		if(!*p)
			return i;
		q = p;
		while(*p && *p!=' ' && *p!='\t')
			p++;
		if(*p)
			*p++ = '\0';
		argv[i++] = argbuf;
		argbuf = expandarg(q, argbuf);
		if(!argbuf)
			return 0;
	}
	return 0;
}

/*
 * copy the arg into the buffer,
 * expanding any environment variables.
 * environment variables are assumed to be
 * names (ie. < ANAMELEN long)
 * the entire argument is expanded to be at
 * most MAXARG long and null terminated
 * the address of the byte after the terminating null is returned
 * any problems cause a 0 return;
 */
static char *
expandarg(char *arg, char *buf)
{
	char env[3+ANAMELEN], *p, *q;
	int fd, n, len;

	n = 0;
	while(p = utfrune(arg, L'$')){
		len = p - arg;
		if(n + len + ANAMELEN >= MAXARG-1)
			return 0;
		memmove(&buf[n], arg, len);
		n += len;
		p++;
		arg = utfrune(p, L'\0');
		q = utfrune(p, L'/');
		if(q && q < arg)
			arg = q;
		q = utfrune(p, L'$');
		if(q && q < arg)
			arg = q;
		len = arg - p;
		if(len >= ANAMELEN)
			continue;
		strcpy(env, "#e/");
		strncpy(env+3, p, len);
		env[3+len] = '\0';
		fd = open(env, OREAD);
		if(fd >= 0){
			len = read(fd, &buf[n], ANAMELEN - 1);
			if(len > 0)
				n += len;
			close(fd);
		}
	}
	len = strlen(arg);
	if(n + len >= MAXARG - 1)
		return 0;
	strcpy(&buf[n], arg);
	return &buf[n+len+1];
}

static int
setenv(char *name, char *val)
{
	int f;
	char ename[ANAMELEN+6];
	long s;

	sprint(ename, "#e/%s", name);
	f = create(ename, OWRITE, 0664);
	if(f < 0)
		return -1;
	s = strlen(val);
	if(write(f, val, s) != s){
		close(f);
		return -1;
	}
	close(f);
	return 0;
}
