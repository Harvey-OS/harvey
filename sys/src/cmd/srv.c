#include <u.h>
#include <libc.h>
#include <bio.h>
#include <auth.h>
#include <fcall.h>

Fcall	hdr;
char	dest[2*NAMELEN];
int	mountflag = MREPL;

void	error(char *);
void	rpc(int, int);
void	post(char*, int);
void	mountfs(char*, int);
int	filter(int);

void
usage(void)
{
	fprint(2, "usage: %s [-abcCmr] [net!]host [srvname [mtpt]]\n", argv0);
		exits("usage");
}

void
ignore(void *a, char *c)
{
	USED(a);
	if(strcmp(c, "alarm") == 0){
		fprint(2, "srv: timeout establishing connection\n");
		exits("timeout");
	}
	noted(NDFLT);
}

void
main(int argc, char *argv[])
{
	int fd;
	char srv[64], mtpt[64];
	char dir[NAMELEN*4];
	char err[ERRLEN];
	char tickets[2*TICKETLEN];
	char *p, *p2;
	int domount, reallymount, record, try;

	notify(ignore);
	alarm(30000);

	domount = 0;
	reallymount = 0;
	record = 0;

	ARGBEGIN{
	case 'a':
		mountflag |= MAFTER;
		domount = 1;
		reallymount = 1;
		break;
	case 'b':
		mountflag |= MBEFORE;
		domount = 1;
		reallymount = 1;
		break;
	case 'c':
		mountflag |= MCREATE;
		domount = 1;
		reallymount = 1;
		break;
	case 'C':
		mountflag |= MCACHE;
		domount = 1;
		reallymount = 1;
		break;
	case 'm':
		domount = 1;
		reallymount = 1;
		break;
	case 'q':
		domount = 1;
		reallymount = 0;
		break;
	case 'r':
		record = 1;
		break;
	default:
		usage();
		break;
	}ARGEND

	if((mountflag&MAFTER)&&(mountflag&MBEFORE))
		usage();


	switch(argc){
	case 1:	/* calculate srv and mtpt from address */
		p = strrchr(argv[0], '/');
		p = p ? p+1 : argv[0];
		snprint(srv, sizeof(srv), "/srv/%.28s", p);
		p2 = strchr(p, '!');
		p2 = p2 ? p2+1 : p;
		snprint(mtpt, sizeof(mtpt), "/n/%.28s", p2);
		break;
	case 2:	/* calculate mtpt from address, srv given */
		snprint(srv, sizeof(srv), "/srv/%.28s", argv[1]);
		p = strrchr(argv[0], '/');
		p = p ? p+1 : argv[0];
		p2 = strchr(p, '!');
		p2 = p2 ? p2+1 : p;
		snprint(mtpt, sizeof(mtpt), "/n/%.28s", p2);
		break;
	case 3:	/* srv and mtpt given */
		domount = 1;
		reallymount = 1;
		snprint(srv, sizeof(srv), "/srv/%.28s", argv[1]);
		snprint(mtpt, sizeof(mtpt), "%.28s", argv[2]);
		break;
	default:
		usage();
	}

	try = 0;
Again:
	try++;
	if(access(srv, 0) == 0){
		if(domount){
			fd = open(srv, ORDWR);
			if(fd >= 0)
				goto Mount;
			remove(srv);
		}
		else{
			fprint(2, "srv: %s already exists\n", srv);
			exits(0);
		}
	}

	strcpy(dest, *argv);

	fd = dial(netmkaddr(*argv, 0, "9fs"), 0, dir, 0);
	if(fd < 0) {
		fprint(2, "srv: dial %s: %r\n", netmkaddr(*argv, 0, "9fs"));
		exits("dial");
	}
	if(record || strstr(dir, "tcp"))
		fd = filter(fd);

	fprint(2, "session...");
	fsession(fd, tickets);
	post(srv, fd);

Mount:
	if(domount == 0 || reallymount == 0)
		exits(0);

	if(amount(fd, mtpt, mountflag, "") < 0){
		err[0] = 0;
		errstr(err);
		if(strstr(err, "hungup") || strstr(err, "timed out")){
			remove(srv);
			if(try == 1)
				goto Again;
		}
		fprint(2, "srv %s: mount failed: %s\n", dest, err);
		exits("mount");
	}
	exits(0);
}

void
post(char *srv, int fd)
{
	int f;
	char buf[128];

	fprint(2, "post...\n");
	f = create(srv, OWRITE, 0666);
	if(f < 0){
		sprint(buf, "create(%s)", srv);
		error(buf);
	}
	sprint(buf, "%d", fd);
	if(write(f, buf, strlen(buf)) != strlen(buf))
		error("write");
}

/* Network on fd1, mount driver on fd0 */
int
filter(int fd)
{
	int p[2];

	if(pipe(p) < 0)
		error("pipe");

	switch(rfork(RFNOWAIT|RFPROC|RFFDG)) {
	case -1:
		error("rfork record module");
	case 0:
		dup(fd, 1);
		close(fd);
		dup(p[0], 0);
		close(p[0]);
		close(p[1]);
		execl("/bin/aux/fcall", "fcall", 0);
		error("exec record module");
	default:
		close(fd);
		close(p[0]);
	}
	return p[1];	
}

void
error(char *s)
{
	fprint(2, "srv %s: %s: %r\n", dest, s);
	exits("srv: error");
}
