/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
 *
 *	debugger
 *
 */

#include "defs.h"
#include "fns.h"

int	printcol = 0;
int	infile = STDIN;
int	maxpos = MAXPOS;

Biobuf	stdout;

void
printc(int c)
{
	dprint("%c", c);
}

/* was move to next f1-sized tab stop; now just print a tab */
int
tconv(Fmt *f)
{
	return fmtstrcpy(f, "\t");
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

	if (maxpos <= printcol)
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
	int n, w;
	char *p;
 	char buf[4096];
	Rune r;
	va_list arg;

	if(mkfault)
		return -1;
	va_start(arg, fmt);
	n = vseprint(buf, buf+sizeof buf, fmt, arg) - buf;
	va_end(arg);
//Bprint(&stdout, "[%s]", fmt);
	Bwrite(&stdout, buf, n);
	for(p=buf; *p; p+=w){
		w = chartorune(&r, p);
		if(r == '\n')
			printcol = 0;
		else
			printcol++;
	}
	return n;
}

void
outputinit(void)
{
	Binit(&stdout, 1, OWRITE);
	fmtinstall('t', tconv);
}
