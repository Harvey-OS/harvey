#include <u.h>
#include <libc.h>

int wrrd;

void
usage(void)
{
	fprint(2, "usage: rdwr [-w] file\n");
	exits("usage");
}

void
main(int argc, char **argv)
{
	int fd;
	char buf[8192];
	int n;

	ARGBEGIN{
	case 'w':
		wrrd = 1;
		break;
	default:
		usage();
	}ARGEND;

	if(argc != 1)
		usage();

	if((fd = open(argv[0], ORDWR)) < 0)
		sysfatal("open: %r");

	if(wrrd){
		n = read(fd, buf, sizeof buf);
		if(n < 0)
			fprint(2, "read error: %r\n");
		else{
			write(1, buf, n);
			print("\n");
		}
	}

	while(print("> "), (n = read(0, buf, 1000)) > 0) {
		seek(fd, 0, 0);
		if(write(fd, buf, n-1) != n-1)	/* n-1: no newline */
			fprint(2, "write error: %r\n");
		seek(fd, 0, 0);
		n = read(fd, buf, sizeof buf);
		if(n < 0)
			fprint(2, "read error: %r\n");
		else{
			write(1, buf, n);
			print("\n");
		}
	}
}
