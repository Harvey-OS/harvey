#include <u.h>
#include <libc.h>
#include <ctype.h>

void	doalldirs(void);
void	dodir(char*);
void	dofile(Dir*);
void	rundir(char*);
char*	file(char*, char);
void	warning(char*, void*);
void	error(char*, void*);
int	returnmail(char**, char*, char*);

#define HUNK 32
char *cmd;
char *root;
int debug;

void
usage(void)
{
	fprint(2, "usage: runq [-a] q-root cmd\n");
	exits("");
}

void
main(int ac, char **av)
{
	char *user;
	int ai;
	char *cp;
	int all;


	user = getenv("user");
	all = 0;

	for(ai = 1; ai < ac; ai++){
		if(av[ai][0] != '-')
			break;
		for(cp = &av[ai][1]; *cp; cp++)
			switch(*cp){
			case 'a':
				all = 1;
				break;
			case 'd':
				debug = 1;
				break;
			}
	}
	if(ac - ai != 2)
		usage();
	root = av[ai];
	cmd = av[ai+1];

	if(chdir(root) < 0)
		error("can't cd to %s", root);

	if(all)
		doalldirs();
	else
		dodir(user);
	exits(0);
}

void
setuser(char *name)
{
	int fd;
	char buf[256];

	/*
	 *  become THAT user
	 */
	fd = create("/dev/user", OWRITE, 0600);
	if(fd < 0)
		error("opening /dev/user", 0);
	if(write(fd, name, strlen(name))<0)
		error("couldn't set user name %s", name);
	close(fd);

	/*
	 *  mount /srv/boot /mnt
	 */
	fd = open("/srv/boot", 2);
	if(fd < 0)
		error("opening /srv/boot", 0);
	if(mount(fd, "/mnt", MREPL, "", "") < 0)
		error("mounting", 0);
	close(fd);

	/*
	 *   bind /mnt/<root> /<root>
	 */
	sprint(buf, "/mnt/%s", root);
	if(bind(buf, root, MREPL) < 0)
		error("binding", 0);
}

int
emptydir(char *name)
{
	int fd;
	long n;
	Dir db2;

	fd = open(name, OREAD);
	if(fd < 0)
		return 1;
	n = read(fd, &db2, sizeof db2);
	close(fd);
	if(n <= 0)
		return 1;
	return 0;
}

/*
 *  run all user directories, must be bootes to do this
 */
void
doalldirs(void)
{
	Dir db[HUNK];
	int fd;
	long i, n;

	fd = open(".", OREAD);
	if(fd == -1){
		warning("reading %s", root);
		return;
	}
	while((n=dirread(fd, db, sizeof db)) > 0){
		n /= sizeof(Dir);
		for(i=0; i<n; i++){
			if(db[i].qid.path&CHDIR){
				if(emptydir(db[i].name))
					continue;
				switch(fork()){
				case -1:
					warning("couldn't fork", 0);
					break;
				case 0:
					if(rfork(RFENVG|RFNAMEG|RFNOTEG)<0)
						error("rfork failed", 0);
/*					setuser(db[i].name); /**/
					dodir(db[i].name);
					exits(0);
				default:
					wait(0);
					break;
				}
			}
		}
	}
	close(fd);
}

/*
 *  cd to a user directory and run it
 */
void
dodir(char *name)
{
	if(chdir(name) < 0){
		warning("cd to %s", name);
		return;
	}
	if(debug)
		fprint(2, "running %s\n", name);
	rundir(name);
	chdir("..");
}

/*
 *  run the current directory
 */
void
rundir(char *name)
{
	Dir db[HUNK];
	int fd;
	long i, n;

	fd = open(".", OREAD);
	if(fd == -1){
		warning("reading %s", name);
		return;
	}
	while((n=dirread(fd, db, sizeof db)) > 0){
		n /= sizeof(Dir);
		for(i=0; i<n; i++){
			if(db[i].name[0]!='C' || db[i].name[1]!='.')
				continue;
			dofile(&db[i]);
		}
	}
	close(fd);
}

/*
 *  try a message
 */
void
dofile(Dir *dp)
{
	Dir d;
	int dfd;
	int cfd;
	char *buf;
	char *cp;
	int ac;
	char **av;
	Waitmsg wm;
	int dtime;
	int efd;

	if(debug)
		fprint(2, "dofile %s\n", dp->name);

	/*
	 *  if no data file, just clean up
	 */
	if(dirstat(file(dp->name, 'D'), &d) < 0){
		remove(dp->name);
		remove(file(dp->name, 'E'));
		return;
	}
	dtime = d.mtime;

	/*
	 *  retry times depend on the age of the errors file
	 */
	if(dirstat(file(dp->name, 'E'), &d) >= 0){
		if(d.mtime - dtime < 60*60){
			/* up to the first hour, try every 15 minutes */
			if(time(0) - d.mtime < 15*60)
				return;
		} else {
			/* after the first hour, try once an hour */
			if(time(0) - d.mtime < 60*60)
				return;
		}
	}

	/*
	 *  open control and data
	 */
	cfd = open(file(dp->name, 'C'), OREAD);
	if(cfd < 0)
		return;
	dfd = open(file(dp->name, 'D'), OREAD);
	if(dfd < 0){
		close(cfd);
		return;
	}

	/*
	 *  make arg list
	 *	- read args into (malloc'd) buffer
	 *	- count args
	 *	- malloc a vector and copy pointers to args into it
	 */
	buf = malloc(dp->length+1);
	if(read(cfd, buf, dp->length) != dp->length){
		warning("reading control file %s\n", dp->name);
		goto out;
	}
	buf[dp->length] = 0;
	for(ac = 0, cp = buf; *cp; ac++){
		while(isspace(*cp))
			cp++;
		while(*cp && !isspace(*cp))
			cp++;
	}
	av = malloc((ac+2)*sizeof(char *));
	av[0] = cmd;
	for(ac = 1, cp = buf; *cp; ac++){
		while(isspace(*cp))
			*cp++ = 0;
		av[ac] = cp;
		if(*cp == '"'){
			cp++;
			while(*cp && *cp!='"')
				cp++;
			if(*cp)
				cp++;
		} else {
			while(*cp && !isspace(*cp))
				cp++;
		}
		while(isspace(*cp))
			*cp++ = 0;
	}
	av[ac] = 0;

	/*
	 *  transfer
	 */
	switch(fork()){
	case -1:
		return;
	case 0:
		close(0);
		dup(dfd, 0);
		close(2);
		efd = open(dp->name, OWRITE);
		if(efd < 0)
			efd = create(file(dp->name, 'E'), OWRITE, 0664);
		if(efd < 0)
			exits("");
		seek(efd, 0, 2);
		close(dfd);
		close(cfd);
		exec(cmd, av);
		error("can't exec %s", cmd);
		break;
	default:
		wait(&wm);
		if(wm.msg[0]){
			if(debug)
				fprint(2, "wm.msg == %s\n", wm.msg);
			if(strstr(wm.msg, "Retry")==0){
				/* return the message and remove it */
				if(returnmail(av, dp->name, wm.msg) == 0){
					remove(file(dp->name, 'D'));
					remove(file(dp->name, 'C'));
					remove(file(dp->name, 'E'));
				}
			} else {
				/* try again later */
			}
		} else {
			/* it worked remove the message */
			remove(file(dp->name, 'D'));
			remove(file(dp->name, 'C'));
			remove(file(dp->name, 'E'));
		}

	}
	return;
out:
	close(cfd);
	close(dfd);
}

/*
 *  return a name starting with the given character
 */
char*
file(char *name, char type)
{
	static char nname[NAMELEN+1];

	strcpy(nname, name);
	nname[0] = type;
	return nname;
}

/*
 *  send back the mail with an error message
 *
 *  return 0 if successful
 */
int
returnmail(char **av, char *name, char *msg)
{
	int pfd[2];
	Waitmsg wm;
	char *sender;
	int fd;
	char buf[256];
	int i;
	long n;

	sender = av[2];

	if(pipe(pfd) < 0)
		return -1;

	switch(fork()){
	case -1:
		return -1;
	case 0:
		close(pfd[1]);
		close(0);
		dup(pfd[0], 0);
		close(pfd[0]);
		execl("/bin/upas/sendmail", "sendmail", sender, 0);
		error("can't exec", 0);
		break;
	default:
		break;
	}

	close(pfd[0]);
	if(av[1]){
		fprint(pfd[1], "Your request ``%20.20s ", av[1]);
		for(n = 3; av[n]; n++)
			fprint(pfd[1], "%s ", av[n]);
	}
	fprint(pfd[1], "'' failed (code %s).\nThe symptom was:\n\n", msg);
	fd = open(file(name, 'E'), OREAD);
	if(fd >= 0){
		for(;;){
			n = read(fd, buf, sizeof(buf));
			if(n <= 0)
				break;
			if(write(pfd[1], buf, n) != n){
				close(fd);
				goto out;
			}
		}
		close(fd);
	}
	fprint(pfd[1], "\nThe request began:\n\n");
	fd = open(file(name, 'D'), OREAD);
	if(fd >= 0){
		for(i=0; i<4*16; i++){
			n = read(fd, buf, sizeof(buf));
			if(n <= 0)
				break;
			if(write(pfd[1], buf, n) != n){
				close(fd);
				goto out;
			}
		}
		close(fd);
	}
	close(pfd[1]);
out:
	wait(&wm);
	return wm.msg[0] ? -1 : 0;
}

/*
 *  print a warning and continue
 */
void
warning(char *f, void *a)
{
	char err[65];
	char buf[256];

	errstr(err);
	sprint(buf, f, a);
	fprint(2, "runq: %s: %s\n", buf, err);	
}

/*
 *  print an error and die
 */
void
error(char *f, void *a)
{
	char err[ERRLEN+1];
	char buf[256];

	errstr(err);
	sprint(buf, f, a);
	fprint(2, "runq: %s: %s\n", buf, err);
	exits(buf);	
}

