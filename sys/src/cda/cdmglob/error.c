#include <u.h>
#include <stdio.h>
#include <libc.h>

char	*ego,		/* program name */
	*filename;	/* current file name */

int	lineno,		/* current line number */
	nerrors;	/* number of nonfatal errors */


void
error0(void)
{
	fprintf (stderr, "%s: ", ego);
	if (filename) {
		fprintf (stderr, "\"%s\"", filename);
		if (lineno > 0)
			fprintf (stderr, ", line %d", lineno);
		fprintf (stderr, ": ");
	}
	else	if (lineno > 0)
			fprintf (stderr, "line %d: ", lineno);
	return;
}


/* PRINTFLIKE1 */

void
error (char *fmt, char *a, char *b, char *c, char *d)
{
	error0();
	fprintf (stderr, fmt, a, b, c, d);
	fprintf (stderr, "\n");
	nerrors++;
	return;
}


/* PRINTFLIKE1 */

void
fatal (char *fmt, char *a, char *b, char *c, char *d)
{
	error (fmt, a, b, c, d);
#ifdef PLAN9
	exits("cdmglob fatal");
#else
	exit (1);
#endif
}


/* PRINTFLIKE1 */

void
warn (char *fmt, char *a, char *b, char *c, char *d)
{
	error0();
	fprintf (stderr, "warning: ");
	fprintf (stderr, fmt, a, b, c, d);
	fprintf (stderr, "\n");
	return;
}
