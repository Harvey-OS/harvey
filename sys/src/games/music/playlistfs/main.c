#include <u.h>
#include <libc.h>
#include <thread.h>
#include <fcall.h>
#include "playlist.h"

int	debug;
char	*user;
int	srvfd[2];
int	aflag;

void
usage(void)
{
	sysfatal("usage: %s [-d bitmask] [-s] [-m mountpoint]", argv0);
}

void
post(char *name, int sfd)
{
	int fd;
	char buf[32];

	fd = create(name, OWRITE, 0666);
	if(fd < 0)
		return;
	sprint(buf, "%d", sfd);
	if(write(fd, buf, strlen(buf)) != strlen(buf))
		sysfatal("srv write: %r");
	close(fd);
}

void
threadmain(int argc, char *argv[])
{
	char *srvfile;
	char *srvpost;
	char *mntpt;
	int i;

	mntpt = "/mnt";
	srvpost = nil;

	rfork(RFNOTEG);

	ARGBEGIN{
	case 'a':
		aflag = 1;
		break;
	case 'm':
		mntpt = ARGF();
		break;
	case 'd':
		debug = strtoul(ARGF(), nil, 0);
		break;
	case 's':
		srvpost = ARGF();
		break;
	default:
		usage();
	}ARGEND

	user = strdup(getuser());

	quotefmtinstall();

	if(debug)
		fmtinstall('F', fcallfmt);

	volumechan = chancreate(sizeof(volume), 1);
	playchan = chancreate(sizeof(Wmsg), 1);
	playlistreq = chancreate(sizeof(Wmsg), 0);	/* No storage! requires rendez-vous */
	workers = chancreate(sizeof(Worker*), 256);
	for(i = 1; i < Nqid; i++)
		files[i].workers = chancreate(sizeof(Worker*), 256);

	if(pipe(srvfd) < 0)
		sysfatal("pipe failed: %r");
	procrfork(srv, nil, 8192, RFFDG);
	close(srvfd[0]);	/* don't deadlock if child fails */

	procrfork(volumeproc, nil, 8192, RFFDG);
	playinit();

	if(srvpost){
		srvfile = smprint("/srv/playlist.%s", srvpost);
		remove(srvfile);
		post(srvfile, srvfd[1]);
		free(srvfile);
	}
	if(mount(srvfd[1], -1, mntpt, MBEFORE, "") < 0)
		sysfatal("mount failed: %r");
	threadexits(nil);
}
