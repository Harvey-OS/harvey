#include <u.h>
#include <libc.h>
#include <auth.h>

void	error(char*, ...);
int	readln(char*, char*, int);
void	usage(void);

void
main(int argc, char *argv[])
{
	char buf[32], pass[32], key[DESKEYLEN], statbuf[DIRLEN];
	int n;

	ARGBEGIN{
	default:
		usage();
	}ARGEND
	if(argc)
		usage();

	if(stat("#b/bitblt", statbuf) < 0){
		fprint(2, "please run netkey only on a terminal\n");
		exits("please run netkey on a terminal");
	}
	readln("Password: ", pass, sizeof pass);
	if(!passtokey(key, pass))
		fprint(2, "warning: bad password -- don't use your pin\n");

	for(;;){
		print("challenge: ");
		n = read(0, buf, sizeof buf - 1);
		if(n <= 0)
			exits(0);
		buf[n] = '\0';
		netcrypt(key, buf);
		print("response: %s\n", buf);
	}
}

int
readln(char *prompt, char *line, int len)
{
	char *p;
	int fd, ctl, n, nr;

	fd = open("/dev/cons", ORDWR);
	if(fd < 0)
		error("couldn't open cons");
	ctl = open("/dev/consctl", OWRITE);
	if(ctl < 0)
		error("couldn't set raw mode");
	write(ctl, "rawon", 5);
	fprint(fd, "%s", prompt);
	nr = 0;
	p = line;
	for(;;){
		n = read(fd, p, 1);
		if(n < 0){
			close(fd);
			close(ctl);
			return -1;
		}
		if(n == 0 || *p == '\n' || *p == '\r'){
			*p = '\0';
			write(fd, "\n", 1);
			close(fd);
			close(ctl);
			return nr;
		}
		if(*p == '\b'){
			if(nr > 0){
				nr--;
				p--;
			}
		}else if(*p == 21){		/* cntrl-u */
			fprint(fd, "\n%s", prompt);
			nr = 0;
			p = line;
		}else{
			nr++;
			p++;
		}
		if(nr == len){
			fprint(fd, "line too long; try again\n%s", prompt);
			nr = 0;
			p = line;
		}
	}
	return -1;
}

void
error(char *fmt, ...)
{
	char buf[8192], *s;

	s = buf;
	s += sprint(s, "netkey: ");
	s = doprint(s, buf + sizeof(buf) / sizeof(*buf), fmt, &fmt + 1);
	*s++ = '\n';
	write(2, buf, s - buf);
	exits(buf);
}

void
usage(void)
{
	fprint(2, "usage: netkey\n");
	exits("usage");
}
