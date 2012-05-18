#include <u.h>
#include <libc.h>

/*
 * NIX optimistic semaphores shared with the kernel testing.
 !	6c -FTVw testksem.c && 6l -o 6.testksem testksem.6
 */


void
usage(void)
{
	fprint(2, "usage: %s [-ud] [n]\n", argv0);
	exits("usage");
}

int	usem;

void
main(int argc, char *argv[])
{
	int isup;
	int fd, n;

	isup = 0;
	ARGBEGIN{
	case 'u':
		isup = 1;
		break;
	case 'd':
		isup = 0;
		break;
	default:
		usage();
	}ARGEND;
	n = 10;
	if(argc == 1)
		n = atoi(argv[0]);
	else if(argc != 0)
		usage();


	fd = open("#=/ctl", ORDWR);
	if(fd < 0)
		sysfatal("devtube: %r");
	if(isup)
		fprint(fd, "up %#p", &usem);
	else
		fprint(fd, "down %#p", &usem);
	if(isup == 0)
		sleep(15*1000);	/* let the kernel block, to check that */
	while(n-- > 0){
		print("user: %s... s=%d\n", isup?"down":"up", usem);
		if(isup)
			downsem(&usem, 0);
		else
			upsem(&usem);
		print("user: %s done\n", isup?"down":"up");
	}
	exits(nil);
}
