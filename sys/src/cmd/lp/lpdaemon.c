#include <u.h>
#include <libc.h>

/* for Plan 9 */
#define LP	"/bin/lp"
/* for Tenth Edition systems	*/
/* #define LP	"/usr/bin/lp"	*/
/* for System V or BSD systems	*/
/* #define LP	"/v/bin/lp"	*/

#define LPDAEMONLOG	"/tmp/lpdaemonl"

#define ARGSIZ 4096

char argstr[ARGSIZ];		/* arguments read from port */
char argvstr[ARGSIZ];		/* arguments after parsing */
char *argvals[ARGSIZ/2+1];	/* pointers to arguments after parsing */
int ascnt = 0, argcnt = 0;	/* number of arguments parsed */

/*
 * predefined
 */
void error(char *s1);

void
main(int argc, char *argv[])
{
	register char *ap, *bp;
	int i, saveflg;

	USED(argc, argv);
	/* get the first line sent and parse it as arguments for lp */
	ap = argstr;
	while((i=read(0, ap, 1)) >= 0) {
		if (i == 0) continue;
		if (*ap++ == '\n') break;
	}
	*ap = '\0';

	ap = argvstr;
	/* setup argv[0] for exec */
	argvals[argcnt++] = ap;
	for (bp=LP; *bp!='\0';) *ap++ = *bp++;
	*ap++ = '\0';

	bp = argstr;
	/* setup the remaining arguments */
	do {
		/* move to next non-white space */
		while(*bp==' '||*bp=='\t'||*bp=='\r'||*bp=='\n')
			++bp;
		if (*bp=='\0') continue;
		/* only accept arguments beginning with -
		 * this is done to prevent the printing of
		 * local files from the destination host
		 */
		if (*bp=='-') {
			argvals[argcnt++] = ap;
			saveflg = 1;
		} else
			saveflg = 0;
		/* move to next white space copying text to argument buffer */
		while (*bp!=' ' && *bp!='\t' && *bp!='\r' && *bp!='\n'
		    && *bp!='\0') {
			*ap = *bp++;
			ap += saveflg;
		}
		*ap = '\0';
		ap += saveflg;
	} while (*bp!='\0');

/*	for (i=0; i<argcnt; i++) {
/*		error(argvals[i]);
/*		error(" ");
/*	}
/*	error("\n");
 */
	exec(LP, argvals);
	error("exec failed\n");
	exits("exec failed");
}

void
error(char *s1)
{
	int fd;

	if((fd=open(LPDAEMONLOG, OWRITE))<0) {
		if((fd=create(LPDAEMONLOG, OWRITE, 0666))<0) {
			return;	/* hopeless, just go away mad */
		}
	}
	seek(fd, 0, 2);
	fprint(fd, s1);
	close(fd);
	return;
}
