#include <newns.c>

void	error(char*);
void	usage(void);
int	getc(void);
int	puts(char*);

void
usage(void)
{
	fprint(2, "usage: login [-u user] [-n namespace_file] [cmd]\n");
	exits("usage");
}

void
main(int argc, char *argv[])
{
	char *err, user[NAMELEN];
	char *namespace = "/lib/namespace";

	user[0] = '\0';
	ARGBEGIN{
	case 'u':
		strncpy(user, ARGF(), NAMELEN);
		user[NAMELEN-1] = '\0';
		break;
	case 'n':
		namespace = ARGF();
		break;
	default:
		usage();
	}ARGEND

	if(argc > 1)
		usage();

	if(err = newns(getuser(), namespace))
		error(err);
	if(argc)
		execl("/bin/rc", "rc", "-c", argv[0]);
	else
		execl("/bin/rc", "rc", "-i", 0);
fprint(2, "cputype=%s user=%s\n", getenv("cputype"), getuser());
	error("can't exec rc");
	exits(0);
}

int
getc(void)
{
	char c;

	if(read(0, &c, 1) != 1)
		return -1;
	return c;
}

int
puts(char *s)
{
	return write(1, s, strlen(s));
}

void
error(char *msg)
{
	fprint(2, "%s: %s: %r\n", argv0, msg);
	exits(msg);
}
