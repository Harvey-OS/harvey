#include <u.h>
#include <libc.h>
#include <bio.h>

int	direction;	/* direction of transfer */
char	*dev;		/* device */
int	bsize;		/* block size */
char	*buf;		/* io buffer */
int	sofar;		/* bytes in buffer */

void	tofloppy(int);
void	fromfloppy(int);

void
usage(void)
{
	fprint(2, "usage:	flio -i floppyfile -b blocksize\n");
	fprint(2, "	flio -o floppyfile -b blocksize\n");
	exits("usage");
}

void
main(int argc, char *argv[])
{
	char *cp;
	int fd, cons, vol;
	Biobuf bin;
	Dir d;

	direction = OREAD;
	bsize = 512;

	ARGBEGIN {
	case 'i':
		direction = OREAD;
		break;
	case 'o':
		direction = OWRITE;
		break;
	case 'b':
		cp = ARGF();
		bsize = atoi(cp);
		if(cp[strlen(cp)-1] == 'k')
			bsize *= 1024;
		break;
	default:
		usage();
	} ARGEND

	if(argc != 1)
		usage();
	dev = argv[0];

	if(dirstat(dev, &d) == 0 && (d.length%bsize)){
		fprint(2, "flio: device length %d not multiple of block size %d\n",
			d.length, bsize);
		exits("bsize");
	}

	cons = open("/dev/cons", ORDWR);
	if(cons < 0){
		fprint(2, "flio: can't open /dev/cons: %r\n");
		exits("/dev/cons");
	}
	Binit(&bin, cons, OREAD);
	buf = malloc(bsize);
	if(buf == 0){
		fprint(2, "flio: out of memory: %r\n");
		exits("mem");
	}

	for(vol = 0; ; vol++){
		do {
			fprint(cons, "Insert %s diskette %d and press return\n", 
				direction==OREAD ? "source" : "destination", vol);
			cp = Brdline(&bin, '\n');
			if(cp == 0)
				exits(0);
			fd = open(dev, direction);
			if(fd < 0)
				fprint(2, "error opening %s: %r\n", dev);
		} while(fd < 0);

		switch(direction){
		case OWRITE:
			tofloppy(fd);
			break;
		case OREAD:
			fromfloppy(fd);
			break;
		}
	}
}

void
tofloppy(int fd)
{
	int n;

	for(;;){
		if(sofar < bsize){
			n = read(0, buf+sofar, bsize-sofar);
			if(n <= 0){
				if(sofar){
					memset(buf+sofar, 0, bsize-sofar);
					sofar = bsize;
				} else
					exits(0);
			}
			sofar += n;
		}
		if(sofar == bsize){
			if(write(fd, buf, bsize) < 0)
				return;
			sofar = 0;
		}
	}
}

void
fromfloppy(int fd)
{
	for(;;){
		if(read(fd, buf, bsize) != bsize)
			return;
		if(write(1, buf, bsize) < 0)
			exits(0);
	}
}
