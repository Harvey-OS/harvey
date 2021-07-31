#include <u.h>
#include <libc.h>

int touch(int, char *);

void
main(int argc, char **argv)
{
	int force = 1;
	int status = 0;

	ARGBEGIN{
	case 'c':	force = 0; break;
	default:	goto usage; break;
	}ARGEND
	if(!*argv){
usage:
		fprint(2, "usage: touch [-c] files\n");
		exits("usage");
	}
	while(*argv)
		status += touch(force, *argv++);
	if(status)
		exits("touch");
	exits(0);
}

touch(int force, char *name)
{
	Dir stbuff;
	char junk[1];
	int fd;

	stbuff.length = 0;
	stbuff.mode = 0666;
	if (dirstat(name, &stbuff) < 0 && force == 0) {
		fprint(2, "touch: %s: cannot stat\n", name);
		return 1;
	}
	if (stbuff.length == 0) {
		if ((fd = create(name, 0, stbuff.mode)) < 0) {
			fprint(2, "touch: %s: cannot create\n", name);
			return 1;
		}
		close(fd);
		return 0;
	}
	if ((fd = open(name, ORDWR)) < 0) {
		fprint(2, "touch: %s: cannot open\n", name);
		return 1;
	}
	if(read(fd, junk, 1) < 1) {
		fprint(2, "touch: %s: read error\n", name);
		close(fd);
		return 1;
	}
	seek(fd, 0L, 0);
	if(write(fd, junk, 1) < 1 ) {
		fprint(2, "touch: %s: write error\n", name);
		close(fd);
		return 1;
	}
	close(fd);
	return 0;
}
