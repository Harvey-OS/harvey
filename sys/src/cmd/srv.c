#include <u.h>
#include <libc.h>
#include <bio.h>
#include <auth.h>
#include <fcall.h>

Fcall	hdr;
char	dest[2*NAMELEN];

void	error(char *);
int	cyclone(void);
void	rpc(int, int);
void	post(char*, int);
void	mountfs(char*, int);

void
usage(void)
{
	fprint(2, "usage: %s [-m] [net!]host [srvname [mtpt]]\n", argv0);
		exits("usage");
}

void
main(int argc, char *argv[])
{
	int fd, cfd;
	char srv[64], mtpt[64];
	char dir[NAMELEN*4];
	char err[ERRLEN];
	char tickets[2*TICKETLEN];
	char *p, *p2;
	int domount, record, try;

	domount = 0;
	record = 0;
	ARGBEGIN{
	case 'm':
		domount = 1;
		break;
	case 'r':
		record = 1;
		break;
	default:
		usage();
		break;
	}ARGEND

	switch(argc){
	case 1:	/* calculate srv and mtpt from address */
		p = strrchr(argv[0], '/');
		p = p ? p+1 : argv[0];
		sprint(srv, "/srv/%.28s", p);
		p2 = strchr(p, '!');
		p2 = p2 ? p2+1 : p;
		sprint(mtpt, "/n/%.28s", p2);
		break;
	case 2:	/* calculate mtpt from address, srv given */
		sprint(srv, "/srv/%.28s", argv[1]);
		p = strrchr(argv[0], '/');
		p = p ? p+1 : argv[0];
		p2 = strchr(p, '!');
		p2 = p2 ? p2+1 : p;
		sprint(mtpt, "/n/%.28s", p2);
		break;
	case 3:	/* srv and mtpt given */
		domount = 1;
		sprint(srv, "/srv/%.28s", argv[1]);
		sprint(mtpt, "%.28s", argv[2]);
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

	if(strncmp(*argv, "cyc!", 4) == 0)
		fd = cyclone();
	else {
		fd = dial(netmkaddr(*argv, 0, "9fs"), 0, dir, &cfd);
		if(fd < 0) {
			fprint(2, "srv: dial %s: %r\n", netmkaddr(*argv, 0, "9fs"));
			exits("dial");
		}
		if(record || strstr(dir, "tcp")) {
			if(write(cfd, "push fcall", 10) < 0) {
				fprint(2, "srv: push fcall %r\n");
				exits(0);
			}
			close(cfd);
		}
	}

	fprint(2, "session...");
	fsession(fd, tickets);
	post(srv, fd);

Mount:
	if(domount == 0)
		exits(0);

	if(amount(fd, mtpt, MREPL, "") < 0){
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

int
cyclone(void)
{
	int fd;

	fd = open("#H/hotrod", ORDWR);
	if(fd < 0)
		error("opening #H/hotrod");
	return fd;
}

void
error(char *s)
{
	fprint(2, "srv %s: %s: %r\n", dest, s);
	exits("srv: error");
}
