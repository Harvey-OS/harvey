#include <u.h>
#include <libc.h>
#include <bio.h>
#include <auth.h>

void	error(char*);
void	usage(void);
int	challuser(char*);
int	readln(char*, char*, int);

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

	while(challuser(user) < 0)
		user[0] = '\0';

	if(err = newns(user, namespace))
		error(err);

	if(argc)
		execl("/bin/rc", "rc", "-lc", argv[0]);
	else
		execl("/bin/rc", "rc", "-li", 0);
	error("can't exec rc");
	exits(0);
}

/*
 *  challenge user
 */
int
challuser(char *user)
{
	char chall[NETCHLEN];
	char nchall[NETCHLEN+32];
	char response[NAMELEN];
	int fd;

	if(*user == 0)
		readln("user: ", user, NAMELEN);
	if(*user == 0)
		return 0;
	fd = getchall(user, chall);
	if(fd < 0)
		return -1;
	sprint(nchall, "challenge: %s\nresponse: ", chall);
	readln(nchall, response, sizeof response);
	if(challreply(fd, user, response) < 0)
		return -1;
	return 0;
}

int
readln(char *prompt, char *line, int len)
{
	char *p;
	int fd, n, nr;

	fd = open("/dev/cons", ORDWR);
	if(fd < 0)
		error("couldn't open cons");
	fprint(fd, "%s", prompt);
	nr = 0;
	p = line;
	for(;;){
		n = read(fd, p, 1);
		if(n < 0){
			close(fd);
			return -1;
		}
		if(n == 0 || *p == '\n' || *p == '\r'){
			*p = '\0';
			close(fd);
			return nr;
		}
		nr++;
		p++;
		if(nr == len){
			fprint(fd, "\n%s", prompt);
			nr = 0;
			p = line;
		}
	}
	return -1;		/* to keep compiler happy */
}

void
usage(void)
{
	fprint(2, "usage: login [-u user] [-n namespace_file] [cmd]\n");
	exits("usage");
}

void
error(char *msg)
{
	fprint(2, "%s: %s: %r\n", argv0, msg);
	exits(msg);
}
