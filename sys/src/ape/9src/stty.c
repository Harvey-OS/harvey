#include <u.h>
#include <libc.h>
#include <tty.h>

typedef struct Mode Mode;
struct Mode
{
	char*	name;
	int	bit;
};

Mode ou[] =
{
	"opost",	OPOST,
	"olcuc",	OLCUC,
	"onlcr",	ONLCR,
	"ocrnl",	OCRNL,
	"onocr",	ONOCR,
	"onlret",	ONLRET,
	"ofill",	OFILL,
	"ofdel",	OFDEL,
	0
};

Mode in[] =
{
	"brkint",	BRKINT,
	"icrnl",	ICRNL,
	"ignbrk",	IGNBRK,
	"igncr",	IGNCR,
	"ignpar",	IGNPAR,
	"inlcr",	INLCR,
	"inpck",	INPCK,
	"istrip",	ISTRIP,
	"ixoff",	IXOFF,
	"ixon",		IXON,
	"parmrk",	PARMRK,
	0
};

Mode lo[] =
{
	"echo",		ECHO,
	"echoe",	ECHOE,
	"echok", 	ECHOK,
	"echonl",	ECHONL,
	"icanon",	ICANON,
	"iexten",	IEXTEN,
	"isig",		ISIG,
	"noflsh",	NOFLSH,
	"tostop",	TOSTOP,
	0
};

Mode cc[] =
{
	"eof",		VEOF,
	"eol",		VEOL,
	"erase",	VERASE,
	"intr",		VINTR,
	"kill",		VKILL,
	"min",		VMIN,
	"quit",		VQUIT,
	"susp",		VSUSP,
	"time",		VTIME,
	"start",	VSTART,
	"stop",		VSTOP,
	0,
};

int	getmode(int, Termios*);
int	setmode(int, Termios*);

char*
ctlchar(char c)
{
	static char buf[10];

	if(c == 0x7f)
		return "DEL";
	if(c == 0)
		return "NUL";
	if(c < 32) {
		buf[0] = '^';
		buf[1] = '@'+c;
		buf[2] = '\0';
		return buf;
	}	
	buf[0] = c;
	buf[1] = '\0';
	return buf;
}

void
showmode(Termios *t)
{
	int i;

	for(i = 0; cc[i].name; i++) {
		switch(cc[i].bit) {
		case VMIN:
		case VTIME:
			if(t->cc[i] != 0)
				print("%s %d ", cc[i].name, t->cc[i]);
			break;
		default:
			print("%s %s ", cc[i].name, ctlchar(t->cc[i]));
			break;
		}
	}
	print("\n");

	for(i = 0; ou[i].name; i++)
		if(ou[i].bit & t->oflag)
			print("%s ", ou[i].name);

	for(i = 0; in[i].name; i++)
		if(in[i].bit & t->iflag)
			print("%s ", in[i].name);

	print("\n");
	for(i = 0; lo[i].name; i++)
		if(lo[i].bit & t->lflag)
			print("%s ", lo[i].name);
	print("\n");
}

int
setreset(char *mode, int *bits, Mode *t)
{
	int i, clr;

	clr = 0;
	if(mode[0] == '-') {
		mode++;
		clr = 1;
	}
	for(i = 0; t[i].name; i++) {
		if(strcmp(mode, t[i].name) == 0) {
			if(clr)
				*bits &= ~t[i].bit;
			else
				*bits |= t[i].bit;

			return 1;
		}
	}
	return 0;
}

int
ccname(char *name)
{
	int i;

	for(i = 0; cc[i].name; i++)
		if(strcmp(cc[i].name, name) == 0)
			return i;

	return -1;
}

void
main(int argc, char **argv)
{
	Termios t;
	int i, stdin, wmo, cc;

	/* Try and get a seek pointer */
	stdin = open("/fd/0", ORDWR);
	if(stdin < 0)
		stdin = 0;

	if(getmode(stdin, &t) < 0) {
		fprint(2, "stty: tiocget %r\n");
		exits("1");
	}

	if(argc < 2) {
		fprint(2, "usage: stty [-a|-g] modes...\n");
		exits("1");
	}
	wmo = 0;
	for(i = 1; i < argc; i++) {
		if(strcmp(argv[i], "-a") == 0) {
			showmode(&t);
			continue;
		}
		if(setreset(argv[i], &t.iflag, in)) {
			wmo++;
			continue;
		}
		if(setreset(argv[i], &t.lflag, lo)) {
			wmo++;
			continue;
		}
		if(setreset(argv[i], &t.oflag, ou)) {
			wmo++;
			continue;
		}
		cc = ccname(argv[i]);
		if(cc != -1 && i+1 < argc) {
			wmo++;
			t.cc[cc] = argv[++i][0];
			continue;
		}
		fprint(2, "stty: bad option/mode %s\n", argv[i]);
		exits("1");
	}

	if(wmo) {
		if(setmode(stdin, &t) < 0) {
			fprint(2, "stty: cant set mode %r\n");
			exits("1");
		}
	}

	exits(0);
}

int
setmode(int fd, Termios *t)
{
	int n, i;
	char buf[256];

	n = sprint(buf, "IOW %4.4ux %4.4ux %4.4ux %4.4ux ",
		t->iflag, t->oflag, t->cflag, t->lflag);
	for(i = 0; i < NCCS; i++)
		n += sprint(buf+n, "%2.2ux ", t->cc[i]);

	if(seek(fd, -2, 0) != -2)
		return -1;

	n = write(fd, buf, n);
	if(n < 0)
		return -1;
	return 0;
}

/*
 * Format is: IOR iiii oooo cccc llll xx xx xx xx ...
 */
int
getmode(int fd, Termios *t)
{
	int n;
	char buf[256];

	if(seek(fd, -2, 0) != -2)
		return -1;

	n = read(fd, buf, 57);
	if(n < 0)
		return -1;

	t->iflag = strtoul(buf+4, 0, 16);
	t->oflag = strtoul(buf+9, 0, 16);
	t->cflag = strtoul(buf+14, 0, 16);
	t->lflag = strtoul(buf+19, 0, 16);

	for(n = 0; n < NCCS; n++)
		t->cc[n] = strtoul(buf+24+(n*3), 0, 16);

	return 0;
}
