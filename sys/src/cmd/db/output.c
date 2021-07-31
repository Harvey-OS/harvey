/*
 *
 *	debugger
 *
 */

#include "defs.h"
#include "fns.h"

int	infile = STDIN;
int	maxpos = MAXPOS;

Biobuf	stdout;

extern int	printcol;

void
printc(int c)
{
	dprint("%c", c);
}

/* move to next f1-sized tab stop */
int
tconv(va_list *arg, Fconv *f)
{
	int n;
	char buf[128];

	USED(arg);
	n = f->f1 - (printcol%f->f1);
	if(n != 0) {
		memset(buf, ' ', n);
		buf[n] = '\0';
		f->f1 = 0;
		strconv(buf, f);
	}
	return 0;
}

/* hexadecimal with initial 0x */
int
myxconv(va_list *arg, Fconv *f)
{
	f->f3 |= 1<<2;
	return numbconv(arg, f);
}

charpos(void)
{
	return printcol;
}

void
flushbuf(void)
{
	if (printcol != 0)
		printc(EOR);
}

void
prints(char *s)
{
	dprint("%s",s);
}

void
newline(void)
{
	printc(EOR);
}

#define	MAXIFD	5
struct {
	int	fd;
	int	r9;
} istack[MAXIFD];
int	ifiledepth;

void
iclose(int stack, int err)
{
	if (err) {
		if (infile) {
			close(infile);
			infile=STDIN;
		}
		while (--ifiledepth >= 0)
			if (istack[ifiledepth].fd)
				close(istack[ifiledepth].fd);
		ifiledepth = 0;
	} else if (stack == 0) {
		if (infile) {
			close(infile);
			infile=STDIN;
		}
	} else if (stack > 0) {
		if (ifiledepth >= MAXIFD)
			error("$<< nested too deeply");
		istack[ifiledepth].fd = infile;
		ifiledepth++;
		infile = STDIN;
	} else {
		if (infile) {
			close(infile); 
			infile=STDIN;
		}
		if (ifiledepth > 0) {
			infile = istack[--ifiledepth].fd;
		}
	}
}

void
oclose(void)
{
	flushbuf();
	Bterm(&stdout);
	Binit(&stdout, 1, OWRITE);
}

void
redirout(char *file)
{
	int fd;

	if (file == 0){
		oclose();
		return;
	}
	flushbuf();
	if ((fd = open(file, 1)) >= 0)
		seek(fd, 0L, 2);
	else if ((fd = create(file, 1, 0666)) < 0)
		error("cannot create");
	Bterm(&stdout);
	Binit(&stdout, fd, OWRITE);
}

void
endline(void)
{

	if (maxpos <= charpos())
		newline();
}

void
flush(void)
{
	Bflush(&stdout);
}

int
dprint(char *fmt, ...)
{
	char buf[4096], *out;
	int n;
	va_list arg;

	if(mkfault)
		return -1;
	va_start(arg, fmt);
	out = doprint(buf, buf+sizeof buf, fmt, arg);
	va_end(arg);
	n = Bwrite(&stdout, buf, (long)(out-buf));
	return n;
}

void
outputinit(void)
{
	Binit(&stdout, 1, OWRITE);
	fmtinstall('t', tconv);
	fmtinstall('x', myxconv);
}
