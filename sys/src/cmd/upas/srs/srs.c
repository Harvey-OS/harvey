#include <u.h>
#include <libc.h>
#include <auth.h>

void
usage(void)
{
	fprint(2, "usage: srs [-d] address\n");
	exits("usage");
}

void
main(int argc, char **argv)
{
	char buf[1024];
	int decode;

	decode = 0;
	ARGBEGIN{
	default:
		usage();
	case 'd':
		decode = 1;
		break;
	}ARGEND
	if(argc != 1)
		usage();

	if(auth_respond(argv[0], strlen(argv[0]), nil, 0, buf, sizeof buf, nil,
			"proto=srs %s", decode ? "decode=1" : "") < 0){
		rerrstr(buf, sizeof buf);
		fprint(2, "%s\n", buf);
		exits(buf);
	}
	print("%s\n", buf);
	exits(0);
}
