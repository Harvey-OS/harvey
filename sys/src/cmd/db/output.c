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
tconv(void *oa, int f1, int f2, int f3, int chr)
{
	USED(oa, chr, f3);
	if(f1 == 0)
		return 0;
	f1 = printcol+(f1-printcol%f1);	/* final column */
	while(printcol < (f1&~7))
		strconv("\t", 0, f2, 0);
	strconv("        "+(8-(f1-printcol)), 0, f2, 0);
	return 0;
}

int	dascase;

/* convert to lower case from upper, according to dascase */
int
Sconv(void *oa, int f1, int f2, int f3, int chr)
{
	char buf[128];
	char *s, *t;

	USED(chr);
	if(dascase){
		for(s=*(char**)oa,t=buf; *t = *s; s++,t++)
			if('A'<=*t && *t<='Z')
				*t += 'a'-'A';
		strconv(buf, f1, f2, f3);
	}else
		strconv(*(char**)oa, f1, f2, f3);
	return sizeof(char*);
}

/* hexadecimal with initial # */
int
myxconv(void *oa, int f1, int f2, int f3, int chr)
{
	/* if unsigned, emit #1234 */
	if((f3&32) || *(long*)oa>=0){
		strconv("#", 1, 1, 0);
		numbconv(oa, f1, f2, f3, chr);
	}else{ /* emit -#1234 */
		*(long*)oa = -*(long*)oa;
		strconv("-#", 2, 2, 0);
		numbconv(oa, f1, f2, f3, chr);
	}
	return sizeof(long);
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
		istack[ifiledepth].r9 = var[9];
		ifiledepth++;
		infile = STDIN;
	} else {
		if (infile) {
			close(infile); 
			infile=STDIN;
		}
		if (ifiledepth > 0) {
			infile = istack[--ifiledepth].fd;
			var[9] = istack[ifiledepth].r9;
		}
	}
}

void
oclose(void)
{
	flushbuf();
	Bclose(&stdout);
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
	Bclose(&stdout);
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

	if(mkfault)
		return -1;
	out = doprint(buf, buf+sizeof buf, fmt, (&fmt+1));
	n = Bwrite(&stdout, buf, (long)(out-buf));
	return n;
}

void
outputinit(void)
{
	Binit(&stdout, 1, OWRITE);
	fmtinstall('t', tconv);
	fmtinstall('x', myxconv);
	fmtinstall('S', Sconv);
}
