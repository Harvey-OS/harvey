#include	<audio.h>

enum { Debug = 0, };

static	int	pid	= 0;	/* outstanding pid of pac decoder */
static	int	pidkilled;
static	int	volfd	= -1;

int
getvolume(void)
{
	char buf[20];
	int i;

	volfd = open(devvolume, ORDWR);
	if(volfd < 0)
		goto bad;

	/*
	 * read and convert the volume
	 */
	seek(volfd, 0, 0);
	memset(buf, 0, sizeof(buf));
	read(volfd, buf, sizeof(buf)-1);
	for(i=0; i<sizeof(buf); i++)
		if(buf[i] >= '0' && buf[i] <= '9')
			return strtoul(buf+i, 0, 0);

bad:
	volfd = -1;
	return -1;
}

void
setvolume(int vol)
{
	seek(volfd, 0, 0);
	fprint(volfd, "%d", vol);
	seek(volfd, 0, 0);
	fprint(volfd, "volume %d", vol);	/* usb is different */
}

void
stopsong(void)
{

	/*
	 * send a 'kill' note to the 'cp' process.
	 */
	if(pid && pidkilled != pid) {
		postnote(PNPROC, pid, "kill\n");
		pidkilled = pid;
	}
}

void
playsong(char *rbin, char *raudio, char *file)
{
	char buf1[200], buf2[200];
	char *prog;
	int p, f;

	/*
	 * create a child that writes the
	 * pac file to the pacman server.
	 */

	if(memcmp(file, "/lib/audio/", 11) == 0)
		file += 11;
	if(memcmp(file, "lib/audio/", 10) == 0)
		file += 10;

	if(pid) {
		print("playsong called with pid still set\n");
		return;
	}

	p = fork();

	/*
	 * child is isolated from parent by
	 * exec of 'cp'. this prevents the
	 * 'kill' note from calling 'exit' and
	 * messing up the parent.
	 */
	if(p != 0) {
		if(p < 0) {
			print("cant fork %r\n");
			p = 0;
		}
		pid = p;
		return;
	}

	if (!Debug) {
		f = open("/dev/null", ORDWR);
		dup(f, 2);
	}
	if(memcmp(strchr(file, 0)-4, ".mp3", 4) == 0)
		goto mp3play;

	prog = "games/pac4dec";
	sprint(buf1, "%s/%s", rbin, prog);
	sprint(buf2, "%s/%s", raudio, file);
	/* audio file names may have had .pac added after index was built */
	if(access(buf2, AEXIST) < 0)
		strcat(buf2, ".pac");
	if(access(buf2, AEXIST) < 0)
		_exits(0);
	if (Debug)
		fprint(2, "%s: running %s: %s %s %s\n",
			argv0, buf1, prog, buf2, devaudio);
	execl(buf1, prog, buf2, devaudio, 0);
	_exits(0);

mp3play:
	f = open(devaudio, OWRITE);
	dup(f, 1);

	prog = "games/mp3dec";
	sprint(buf1, "%s/%s", rbin, prog);
	sprint(buf2, "%s/%s", raudio, file);
	if(access(buf2, AEXIST) < 0)
		_exits(0);
	if (Debug)
		fprint(2, "execl %s %s %s %s\n", buf1, prog, "-s", buf2);
	execl(buf1, prog, "-s", buf2, 0);
	_exits(0);
}

int
songstopped(uchar *b)
{
	int p;

	/*
	 * verify that the wait message
	 * arrived is that of the 'cp' process
	 */
	p = strtoul((char*)b, 0, 0);
	if(p == pid) {
		pid = 0;
		return 1;
	}
	return 0;
}
