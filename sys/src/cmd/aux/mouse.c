#include <u.h>
#include <libc.h>

enum
{
	Sleep500	= 500,
	Sleep1000	= 1000,
	Sleep2000	= 2000,
};

char *speeds[] =
{
	"b1200",
	"b2400",
	"b4800",
	"b9600",
	0,
};

#define DEBUG if(debug)

int can9600;	/* true if type W mouse can be set to 9600 */
int debug;

static void
usage(void)
{
	fprint(2, "%s: usage: %s [device]\n", argv0, argv0);
	exits("usage");
}

static void
catch(void *a, char *msg)
{
	USED(a, msg);
	noted(NCONT);
}

static void
dumpbuf(char *buf, int nbytes, char *s)
{
	print(s);
	if(nbytes){
		while(nbytes--)
			print("#%ux ", *buf++ & 0xFF);
	}
	print("\n");
}

static int
readbyte(int fd)
{
	uchar c;
	char buf[ERRLEN];

	alarm(200);
	if(read(fd, &c, sizeof(c)) == -1){
		alarm(0);
		errstr(buf);
		if(strcmp(buf, "interrupted") == 0)
			return -1;
		fprint(2, "%s: readbyte failed - %s\n", argv0, buf);
		exits("read");
	}
	alarm(0);
	return c;
}

static int
slowread(int fd, char *buf, int nbytes)
{
	char *p;
	int c;

	for(p = buf; nbytes && (c = readbyte(fd)) != -1; *p++ = c, nbytes--)
		;
	*p = 0;
	return p-buf;
}

static void
toggleRTS(int fd)
{
	/*
	 *
	 * reset the mouse (toggle RTS)
	 * must be >100mS
	 */
	fprint(fd, "d0");
	fprint(fd, "r0");
	sleep(Sleep500);
	fprint(fd, "d1");
	fprint(fd, "r1");
	sleep(Sleep500);
}

static void
setupeia(int fd, char *baud, char *bits)
{
	/*
	 * set the speed to 1200/2400/4800/9600 baud,
	 * 7/8-bit data, one stop bit and no parity
	 */
	DEBUG print("setupeia(%s,%s)\n", baud, bits);
	fprint(fd, baud);
	fprint(fd, bits);
	fprint(fd, "s1");
	fprint(fd, "pn");
}

/*
 *  check for a types M, M3, & W
 *
 *  we talk to all these mice using 1200 baud
 */
int
MorW(int ctl, int data)
{
	char buf[256];
	int c;

	/*
	 * set up for type M, V or W
	 * flush any pending data
	 */
	setupeia(ctl, "b1200", "l7");
	toggleRTS(ctl);
	while((c = slowread(data, buf, sizeof(buf))) > 0)
		DEBUG dumpbuf(buf, c, "flush: ");
	toggleRTS(ctl);

	/*
	 * see if there's any data from the mouse
	 * (type M, V and W mice)
	 */
	c = slowread(data, buf, sizeof(buf));
	DEBUG dumpbuf(buf, c, "check M: ");

	/*
	 * type M, V and W mice return "M" or "M3" after reset.
	 * check for type W by sending a 'Send Standard Configuration'
	 * command, "*?".
	 */
	if(c > 0 && buf[0] == 'M'){
		write(data, "*?", 2);
		c = slowread(data, buf, sizeof(buf));
		DEBUG dumpbuf(buf, c, "check W: ");
		/*
		 * 4 bytes back
		 * indicates a type W mouse
		 */
		if(c >= 4){
			if(buf[1] & (1<<4))
				can9600 = 1;
			setupeia(ctl, "b1200", "l8");
			write(data, "*U", 2);
			c = slowread(data, buf, sizeof(buf));
			DEBUG dumpbuf(buf, c, "check W: ");
			return 'W';
		}
		return 'M';
	}
	return 0;
}

/*
 *  check for type C by seeing if it responds to the status
 *  command "s".  the mouse is at an unknown speed so we
 *  have to check all possible speeds.
 */
int
C(int ctl, int data)
{
	char **s;
	int c;
	char buf[256];
	
	sleep(100);
	for(s = speeds; *s; s++){
		DEBUG print("%s\n", *s);
		setupeia(ctl, *s, "l8");
		write(data, "s", 1);
		c = slowread(data, buf, sizeof(buf));
		DEBUG dumpbuf(buf, c, "check C: ");
		if(c >= 1 && (*buf & 0xBF) == 0x0F){
			sleep(100);
			write(data, "*n", 2);
			sleep(100);
			setupeia(ctl, "b1200", "l8");
			write(data, "s", 1);
			c = slowread(data, buf, sizeof(buf));
			DEBUG dumpbuf(buf, c, "recheck C: ");
			if(c >= 1 && (*buf & 0xBF) == 0x0F){
				write(data, "U", 1);
				return 'C';
			}
		}
		sleep(100);
	}

	return 0;
}

char *bauderr = "mouse: can't set baud rate, mouse at 1200\n";

void
Cbaud(int ctl, int data, int baud)
{
	char buf[32];

	switch(baud){
	case 0:
	case 1200:
		return;
	case 2400:
		buf[1] = 'o';
		break;
	case 4800:
		buf[1] = 'p';
		break;
	case 9600:
		buf[1] = 'q';
		break;
	default:
		fprint(2, bauderr);
		return;
	}

	buf[0] = '*';
	buf[2] = 0;
	sleep(100);
	write(data, buf, 2);
	sleep(100);
	write(data, buf, 2);
	sprint(buf, "b%d", baud);
	setupeia(ctl, buf, "l8");
}

void
Wbaud(int ctl, int data, int baud)
{
	char buf[32];

	switch(baud){
	case 0:
	case 1200:
		return;
	case 9600:
		if(can9600)
			break;
		/* fall through */
	default:
		fprint(2, bauderr);
		return;
	}
	write(data, "*q", 2);
	setupeia(ctl, "b9600", "l8");
	slowread(data, buf, sizeof(buf));
}

void
main(int argc, char *argv[])
{
	char *p;
	int baud;
	int conf, ctl, data, def, type;
	char buf[256];

	def = 0;
	baud = 0;
	ARGBEGIN{
	case 'b':
		baud = atoi(ARGF());
		break;
	case 'd':
		p = ARGF();
		def = *p;
		break;
	case 'D':
		debug = 1;
		break;
	default:
		usage();
	}ARGEND

	p = "0";
	if(argc)
		p = *argv;

	if((conf = open("#b/mousectl", OWRITE)) == -1){
		fprint(2, "%s: can't open #b/mousectl - %r\n", argv0);
		exits("open #b");
	}

	if(strcmp(p, "ps2") == 0){
		if(write(conf, "ps2", 3) < 0){
			fprint(2, "%s: error setting mouse type - %r\n", argv0);
			exits("write conf");
		}
		exits(0);
	}

	sprint(buf, "#t/eia%sctl", p);
	if((ctl = open(buf, ORDWR)) == -1){
		fprint(2, "%s: can't open %s - %r\n", argv0, buf);
		exits("open ctl");
	}
	sprint(buf, "#t/eia%s", p);
	if((data = open(buf, ORDWR)) == -1){
		fprint(2, "%s: can't open %s - %r\n", argv0, buf);
		exits("open data");
	}

	notify(catch);

	type = MorW(ctl, data);
	if(type == 0)
		type = C(ctl, data);
	if(type == 0){
		/* try the default */
		switch(def){
		case 'C':
			setupeia(ctl, "b1200", "l8");
			break;
		case 'M':
			setupeia(ctl, "b1200", "l7");
			break;
		}
		type = def;
	}

	sprint(buf, "serial %s", p);
	switch(type){
	case 0:
		print("Unknown mouse type\n");
		exits("no mouse");
	case 'C':
		print("Logitech 5 byte mouse\n");
		Cbaud(ctl, data, baud);
		break;
	case 'W':
		print("Type W mouse\n");
		Wbaud(ctl, data, baud);
		break;
	case 'M':
		print("Microsoft compatible mouse\n");
		strcat(buf, " M");
		break;
	}
	DEBUG fprint(2, "mouse configured as '%s'\n", buf);
	if(write(conf, buf, strlen(buf)) < 0){
		fprint(2, "%s: error setting mouse type - %r\n", argv0);
		exits("write conf");
	}
	exits(0);
}
