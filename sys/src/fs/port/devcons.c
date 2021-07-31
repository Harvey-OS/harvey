#include	"all.h"

static
struct
{
	Lock;
	uchar	buf[4000];
	uchar	*in;
	uchar	*out;
	int	printing;
} printq;

static
struct
{
	Lock;
	Rendez;
	uchar	buf[500];
	uchar	*in;
	uchar	*out;
	int	reading;
} readq;

void
printinit(void)
{

	lock(&printq);		/* allocate lock */
	printq.in = printq.buf;
	printq.out = printq.buf;
	unlock(&printq);

	lock(&readq);		/* allocate lock */
	readq.in = readq.buf;
	readq.out = readq.buf;
	unlock(&readq);

	duartinit();
}

/*
 * Put a string on the console.
 * n bytes of s are guaranteed to fit in the buffer and is ready to print.
 * Must be called splhi() and with printq locked.
 * If early in booting (predawn) poll output.
 */
static void
puts(char *s, int n)
{

	if(predawn) {
		while(n > 0) {
			duartxmit(*s++ & 0xFF);
			delay(5);
			n--;
		}
		return;
	}
	if(!printq.printing){
		printq.printing = 1;
		duartstart(*s++ & 0xFF);
		n--;
	}
	memmove(printq.in, s, n);
	printq.in += n;
	if(printq.in >= printq.buf+sizeof(printq.buf))
		printq.in = printq.buf;
}

/*
 * Print a string on the console.  This is the high level routine
 * with a queue to the interrupt handler.  BUG: There is no check against
 * overflow.
 */
void
putstrn(char *str, int n)
{
	int s, m;
	char *t;

	s = splhi();
	lock(&printq);
	while(n > 0){
		if(*str == '\n')
			puts("\r", 1);
		m = printq.buf+sizeof(printq.buf) - printq.in;
		if(n < m)
			m = n;
		t = memchr(str+1, '\n', m-1);
		if(t)
			if(t-str < m)
				m = t - str;
		puts(str, m);
		n -= m;
		str += m;
	}
	unlock(&printq);
	splx(s);
}

/*
 * get character to print at interrupt time
 * always called splhi from proc 0
 */
conschar(void)
{
	uchar *p;
	int c, s;

	s = splhi();
	lock(&printq);
	p = printq.out;
	if(p == printq.in) {
		printq.printing = 0;
		c = -1;
	} else {
		c = *p++;
		if(p >= printq.buf+sizeof(printq.buf))
			p = printq.buf;
		printq.out = p;
	}
	unlock(&printq);
	splx(s);
	return c;
}

/*
 * dispose of input character at interrupt time
 * always called splhi from proc 0
 */
void
kbdchar(int c)
{
	int s;
	uchar *p;
	uchar ch;

	if(c == ('T' - '@')) {			/* ^t */
		for(s=0; s<=conf.nproc; s++)
			dotrace(0);
		return;
	}
	s = splhi();
	lock(&readq);
	if(readq.reading)
		goto out;
	if(c == ('U' - '@')) {
		if(echo)
			putstrn("^U\n", 3);	/* echo */
		readq.in = readq.out;
		goto out;
	}
	if(c == '\r')
		c = '\n';
	ch = c;
	if(echo)
		putstrn((char*)&ch, 1);		/* echo */
	p = readq.in;
	*p++ = c;
	if(p >= readq.buf+sizeof(readq.buf))
		p = readq.buf;
	readq.in = p;
	if(c == '\n' || c == ('D' - '@')) {
		readq.reading = 1;
		wakeup(&readq);
	}
out:
	unlock(&readq);
	splx(s);
}

int
rawchar(int d)
{
	int c;

	for(;;) {
		c = duartrecv();
		if(c) {
			if(c == '\r')
				c = '\n';
			if(c == '\n') {
				duartxmit('\r');
				delay(10);
			}
			duartxmit(c);
			return c;
		}
		delay(1);
		if(d) {
			d--;
			if(d <= 0)
				break;
		}
	}
	return 0;
}

int
reading(void *a)
{

	USED(a);
	return readq.reading;
}


/*
 * get a character from the console
 * this is the high level routine with
 * a queue to the interrupt handler
 */
int
getc(void)
{
	int c, s;
	uchar *p;

	c = 0;

loop:
	sleep(&readq, reading, 0);
	s = splhi();
	lock(&readq);
	p = readq.out;
	if(readq.in == p) {
		readq.reading = 0;
		unlock(&readq);
		splx(s);
		goto loop;
	}
	if(readq.in != p) {
		c = *p++;
		if(p == readq.buf+sizeof(readq.buf))
			p = readq.buf;
		readq.out = p;
	}
	unlock(&readq);
	splx(s);
	return c;
}

int
sprint(char *s, char *fmt, ...)
{

	return donprint(s, s+PRINTSIZE, fmt, (&fmt+1)) - s;
}

int
print(char *fmt, ...)
{
	char buf[PRINTSIZE];
	int n;

	n = donprint(buf, buf+sizeof(buf), fmt, (&fmt+1)) - buf;
	putstrn(buf, n);
	return n;
}

void
panic(char *fmt, ...)
{
	char buf[PRINTSIZE];
	int n;

	lights(Lpanic, 1);
	dumpstack(u);
	strcpy(buf, "panic: ");
	n = donprint(buf+7, buf+sizeof(buf)-1, fmt, (&fmt+1)) - buf;
	buf[n] = '\n';
	putstrn(buf, n+1);
	if(!predawn){
		spllo();
		prflush();
	}
	exit();
}

void
prflush(void)
{
	if(predawn)
		return;
	while(printq.printing)
		delay(100);
}
