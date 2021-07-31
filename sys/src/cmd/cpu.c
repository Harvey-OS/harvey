/*
 * cpu.c - Make a connection to a cpu server
 *
 *	   Invoked by listen as 'cpu -R | -L service net netdir'
 *	    	   by users  as 'cpu [-p] [-h system] [-c cmd args ...]'
 */

#include <u.h>
#include <libc.h>
#include <bio.h>
#include <auth.h>

void	remoteside(void);
void	fatal(int, char*, ...);
void	noteproc(char*);
void	catcher(void*, char*);
void	remotenote(void);
void	usage(void);
void	writestr(int, char*, char*, int);
void	readstr(int, char*, int, char*);
char	*rexcall(int*, char*, char*);

int 	notechan;
char	system[32];
int	cflag;
int	hflag;

char	*srvname = "cpu";
char	*notesrv = "cpunote";
char	*exportfs = "/bin/exportfs";

void
main(int argc, char **argv)
{
	char dat[128], buf[128], cmd[128], *p, *err;
	int data;

	ARGBEGIN{
	case 'R':				/* From listen */
		remoteside();
		break;
	case 'N':
		remotenote();
		break;
	case 'h':
		hflag++;
		p = ARGF();
		if(p==0)
			usage();
		strcpy(system, p);
		break;
	case 'c':
		cflag++;
		cmd[0] = '!';
		cmd[1] = '\0';
		while(p = ARGF()) {
			strcat(cmd, " ");
			strcat(cmd, p);
		}
		break;
	default:
		usage();
	}ARGEND;

	if(argv);
	if(argc != 0)
		usage();

	if(hflag == 0) {
		p = getenv("cpu");
		if(p == 0)
			fatal(0, "set $cpu");
		strcpy(system, p);
	}

	if(err = rexcall(&data, system, srvname))
		fatal(1, "%s: %s", err, system);

	/* Tell the remote side the command to execute and where our working directory is */
	if(cflag)
		writestr(data, cmd, "command", 0);
	if(getwd(dat, sizeof(dat)) == 0)
		writestr(data, "NO", "dir", 0);
	else
		writestr(data, dat, "dir", 0);

	readstr(data, buf, sizeof(buf), "pid");
	noteproc(buf);

	/* Wait for the other end to execute and start our file service
	 * of /mnt/term */
	readstr(data, buf, sizeof(buf), "FS");
	if(strncmp("FS", buf, 2) != 0) {
		print("remote cpu: %s", buf);
		exits(buf);
	}

	/* Begin serving the gnot namespace */
	close(0);
	dup(data, 0);
	close(data);
	execl(exportfs, exportfs, 0);
	fatal(1, "starting exportfs");
}

void
fatal(int syserr, char *fmt, ...)
{
	char buf[ERRLEN];

	doprint(buf, buf+sizeof(buf), fmt, (&fmt+1));
	if(syserr)
		fprint(2, "cpu: %s: %r\n", buf);
	else
		fprint(2, "cpu: %s\n", buf);
	exits(buf);
}

/* Invoked with stdin, stdout and stderr connected to the network connection */
void
remoteside(void)
{
	char *err, user[NAMELEN], home[128], buf[128], xdir[128], cmd[128];
	int i, n, fd, badchdir, gotcmd;

	if(err = srvauth(user))
		fatal(1, err);
	if(err = newns(user, 0))
		fatal(1, err);

	/* Set environment values for the user */
	putenv("user", user);
	sprint(home, "/usr/%s", user);
	putenv("home", home);

	/* Now collect invoking cpus current directory or possibly a command */
	gotcmd = 0;
	readstr(0, xdir, sizeof(xdir), "dir/cmd");
	if(xdir[0] == '!') {
		strcpy(cmd, &xdir[1]);
		gotcmd = 1;
		readstr(0, xdir, sizeof(xdir), "dir");
	}

	/* Establish the new process at the current working directory of the
	 * gnot */
	badchdir = 0;
	if(strcmp(xdir, "NO") == 0)
		chdir(home);
	else if(chdir(xdir) < 0) {
		badchdir = 1;
		chdir(home);
	}

	sprint(buf, "%d", getpid());
	writestr(1, buf, "pid", 0);

	/* Start the gnot serving its namespace */
	writestr(1, "FS", "FS", 0);
	writestr(1, "/", "exportfs dir", 0);

	n = read(1, buf, sizeof(buf));
	if(n != 2 || buf[0] != 'O' || buf[1] != 'K')
		exits("remote tree");

	fd = dup(1, -1);
	if(mount(fd, "/mnt/term", MREPL, "", "") < 0)
		exits("mount failed");

	close(fd);

	for(i = 0; i < 3; i++)
		close(i);

	if(open("/mnt/term/dev/cons", OREAD) != 0)
		exits("open stdin");
	if(open("/mnt/term/dev/cons", OWRITE) != 1)
		exits("open stdout");
	dup(1, 2);

	if(badchdir)
		print("cpu: failed to chdir to '%s'\n", xdir);

	if(gotcmd)
		execl("/bin/rc", "rc", "-lc", cmd, 0);
	else
		execl("/bin/rc", "rc", "-li", 0);
	fatal(1, "exec shell");
}

void
noteproc(char *pid)
{
	char cmd[NAMELEN], syserr[ERRLEN], *err;
	int notepid, n;
	Waitmsg w;

	notepid = fork();
	if(notepid < 0)
		fatal(1, "forking noteproc");
	if(notepid == 0)
		return;

	notify(catcher);

	if(err = rexcall(&notechan, system, notesrv))
		fprint(2, "cpu: can't dial for notify: %s: %r\n", err);
	else{
		sprint(cmd, "%s", pid);
		writestr(notechan, cmd, "notepid", 0);
	}

	for(;;) {
		n = wait(&w);
		if(n < 0) {
			errstr(syserr);
			if(strcmp(syserr, "interrupted") != 0){
				fprint(2, "cpu: wait: %s\n", syserr);
				exits("waiterr");
			}
		}
		if(n == notepid)
			break;
	}
	exits(w.msg);
}

void
catcher(void *a, char *text)
{
	if(a);
	writestr(notechan, text, "catcher", 1);
	noted(NCONT);
}

void
remotenote(void)
{
	char buf[128], *err;
	int pid;

	if(err = srvauth(buf))
		exits(err);
	readstr(0, buf, sizeof(buf), "read pid");
	pid = -atoi(buf);
	for(;;) {
		readstr(0, buf, sizeof(buf), 0);
		if(postnote(pid, buf) < 0)
			exits("posting");
	}
}

void
usage(void)
{
	fprint(2, "usage: cpu [-h system] [-c cmd args ...]\n");
	exits("usage");
}

char*
rexcall(int *fd, char *host, char *service)
{
	char *err;
	char *na;

	na = netmkaddr(host, 0, service);
	if((*fd = dial(na, 0, 0, 0)) < 0)
		return "can't dial";
	if(err = auth(*fd, na))
		return err;
	return 0;
}

void
writestr(int fd, char *str, char *thing, int ignore)
{
	int l, n;

	l = strlen(str);
	n = write(fd, str, l+1);
	if(!ignore && n < 0)
		fatal(1, "writing network: %s", thing);
}

void
readstr(int fd, char *str, int len, char *thing)
{
	int n;

	while(len) {
		n = read(fd, str, 1);
		if(n < 0) {
			if(thing == 0)
				exits("readstr");
			fatal(1, "reading network: %s", thing);
		}
		if(*str == '\0')
			return;
		str++;
		len--;
	}
	fatal(0, "%s: too long", thing);
}
