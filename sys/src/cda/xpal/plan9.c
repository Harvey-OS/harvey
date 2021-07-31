#include <u.h>
#include <libc.h>

int yydebug; /* ignored by plan9 yacc */

char LIBRARY[] = "/sys/lib/cda/library.paddle";

void exit(int code)
{
	exits(code ? "abnormal exit" : (char *) 0);
}

#define	SIZE	1024
#define	FUNSIGN	4
#define	FSHORT	2
#define	FLONG	1
#define	PTR	sizeof (char *)
#define	SHORT	sizeof (int)
#define	INT	sizeof (int)
#define	LONG	sizeof (long)
#define	FLOAT	sizeof (double)
#define	FDIGIT	30
#define	FDEFLT	8
#define	IDIGIT	40
#define	MAXCONV	30

static int
Numbconv(char *o, Fconv *fp, int b)
{
	char s[IDIGIT];
	register long v;
	register int i, f, n, r;
	switch(fp->f3 & (FLONG|FSHORT|FUNSIGN)) {
	case FLONG:
		v = *(long*)o;
		r = LONG;
		break;

	case FUNSIGN|FLONG:
		v = *(unsigned long*)o;
		r = LONG;
		break;

	case FSHORT:
		v = *(short*)o;
		r = SHORT;
		break;

	case FUNSIGN|FSHORT:
		v = *(unsigned short*)o;
		r = SHORT;
		break;

	default:
		v = *(int*)o;
		r = INT;
		break;

	case FUNSIGN:
		v = *(unsigned*)o;
		r = INT;
		break;
	}
	f = 0;
	if(!(fp->f3 & FUNSIGN) && v < 0) {
		v = -v;
		f = 1;
	}
	s[IDIGIT-1] = 0;
	for(i = IDIGIT-2; i >= 1; i--) {
		n = (unsigned long)v % b;
		n += '0';
		if(n > '9')
			n += 'A' - ('9'+1);
		s[i] = n;
		v = (unsigned long)v / b;
		if(fp->f2 >= 0 && i >= IDIGIT-fp->f2)
			continue;
		if(v <= 0)
			break;
	}
	if(f)
		s[--i] = '-';
	strconv(s+i, fp);
	return r;
}

static
int
xconv(void *o, Fconv *fp)
{
	int r;
	r = Numbconv(o, fp, 16);
	return r;
}

void
capxconv(void)
{
	fmtinstall('x', xconv);
}
