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
 * Conditionally compiled routines for setting up and reading the line. Things
 * were getting out of hand with all the ifdefs, and even though this defeats
 * part of the purpose of conditional complilation directives, I think it's easier
 * to follow this way. Thanks to Alan Buckwalter for the System V DKHOST code.
 *
 * postio now can be run as separate read and write processes, but requires that
 * you write a procedure called resetline() and perhaps modify readline() some.
 * I've already tested the code on System V and it seems to work. Ninth Edition
 * and BSD code may be missing.
 *
 * By request I've changed the way some of the setupline() procedures (eg. in the
 * System V implementation) handle things when no line has been given. If line is
 * NULL the new setupline() procedures try to continue, assuming whoever called
 * postio connected stdout to the printer. Things will only work if we can read
 * and write stdout!
 *
 */

#include <stdio.h>
#include <ctype.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <errno.h>

#include "ifdef.h"			/* conditional header file inclusion */
#include "gen.h"			/* general purpose definitions */

FILE	*fp_ttyi, *fp_ttyo;
char	*ptr = mesg;

extern int	window_size;

/*****************************************************************************/

#ifdef SYSV
setupline()

{

    struct termio	termio;

/*
 *
 * Line initialization for SYSV. For now if no line is given (ie. line == NULL )
 * we continue on as before using stdout as ttyi and ttyo. Doesn't work when we're
 * running in interactive mode or forcing stuff that comes back from the printer
 * to stdout. Both cases are now caught by a test that's been added to routine
 * initialize(). The change is primarily for the version of lp that's available
 * with SVR3.2.
 *
 */

#ifdef DKHOST
    if ( line != NULL && *line != '/' )  {
	if ( strncmp(line, "DK:", 3) == 0 )
	    line += 3;
	dkhost_connect();
#ifdef DKSTREAMS
	if ( ioctl(ttyi, I_PUSH, DKSTREAMS) == -1 )
	    error(FATAL, "ioctl error - %s", DKSTREAMS);
	if ( ioctl(ttyi, I_PUSH, "ldterm") == -1 )
	    error(FATAL, "ioctl error - ldterm");
#endif
    } else
#endif

    if ( line == NULL )
	ttyi = fileno(stdout);
    else if ( (ttyi = open(line, O_RDWR)) == -1 )
	error(FATAL, "can't open %s", line);

    if ( (ttyo = dup(ttyi)) == -1 )
	error(FATAL, "can't dup file descriptor for %s", line);

    if ( stopbits == 1 )
	stopbits = 0;
    else stopbits = CSTOPB;

    if ( fcntl(ttyi, F_SETFL, O_NDELAY) == -1 )
	error(FATAL, "fcntl error - F_SETFL");

    if ( ioctl(ttyi, TCGETA, &termio) == -1 )
	error(FATAL, "ioctl error - TCGETA");

    termio.c_iflag = IXON | IGNCR;
    termio.c_oflag = 0;
    termio.c_cflag = HUPCL | CREAD | CS8 | stopbits | baudrate;
    termio.c_lflag = 0;
    termio.c_cc[VMIN] = termio.c_cc[VTIME] = 0;

    if ( ioctl(ttyi, TCSETA, &termio) == -1 )
	error(FATAL, "ioctl error - TCSETA");

    if ( ioctl(ttyi, TCFLSH, 2) == -1 )
	error(FATAL, "ioctl error - TCFLSH");

    fp_ttyi = fdopen(ttyi, "r");

}   /* End of setupline */

/*****************************************************************************/

resetline()

{

    int			flags;		/* for turning O_NDELAY off */
    struct termio	termio;		/* so we can reset flow control */

/*
 *
 * Only used if we're running the program as separate read and write processes.
 * Called from split() after the initial connection has been made and returns
 * TRUE if two processes should work. Don't know if the O_NDELAY stuff is really
 * needed, but setting c_cc[VMIN] to 1 definitely is. If we leave it be (as a 0)
 * the read in readline() won't block!
 *
 */

    if ( (flags = fcntl(ttyi, F_GETFL, 0)) == -1 )
	error(FATAL, "fcntl error - F_GETFL");

    flags &= ~O_NDELAY;

    if ( fcntl(ttyi, F_SETFL, flags) == -1 )
	error(FATAL, "fcntl error - F_SETFL");

    if ( ioctl(ttyi, TCGETA, &termio) == -1 )
	error(FATAL, "ioctl error - TCGETA");

    termio.c_iflag &= ~IXANY;
    termio.c_iflag |= IXON | IXOFF;
    termio.c_cc[VMIN] = 1;
    termio.c_cc[VTIME] = 0;

    if ( ioctl(ttyi, TCSETA, &termio) == -1 )
	error(FATAL, "ioctl error - TCSETA");

    return(TRUE);

}   /* End of resetline */

/*****************************************************************************/

setupstdin(mode)

    int		mode;			/* what to do with stdin settings */

{

    struct termio		termio;

    static int			saved = FALSE;
    static struct termio	oldtermio;

/*
 *
 * Save (mode = 0), reset (mode = 1), or restore (mode = 2) the tty settings for
 * stdin. Expect something like raw mode with no echo will be set up. Explicit
 * code to ensure blocking reads probably isn't needed because blocksize is set
 * to 1 when we're in interactive mode, but I've included it anyway.
 *
 */

    if ( interactive == TRUE )
	switch ( mode )  {
	    case 0:
		if ( isatty(0) != 1 )
		    error(FATAL, "stdin not a terminal - can't run interactive mode");
		if ( ioctl(0, TCGETA, &oldtermio) == -1 )
		    error(FATAL, "can't save terminal settings");
		saved = TRUE;
		break;

	    case 1:
		termio = oldtermio;
		termio.c_lflag &= ~(ICANON | ECHO | ECHOE | ECHOK | ECHONL);
		termio.c_cc[VMIN] = 1;
		termio.c_cc[VTIME] = 0;
		ioctl(0, TCSETA, &termio);
		break;

	    case 2:
		if ( saved == TRUE )
		    ioctl(0, TCSETA, &oldtermio);
		break;
	}   /* End switch */

}   /* End of setupstdin */

/*****************************************************************************/

readline()

{

    int		n;			/* read() return value */
    int		ch;			/* for interactive mode */

    static int	tries = 0;		/* consecutive times read returned 0 */

/*
 *
 * Reads characters coming back from the printer on ttyi up to a newline (or EOF)
 * or until no more characters are available. Characters are put in mesg[], the
 * string is terminated with '\0' when we're done with a line and TRUE is returned
 * to the caller. If complete line wasn't available FALSE is returned. Interactive
 * mode should loop here forever, except during start(), echoing characters to
 * stdout. If it happens to leave FALSE should be returned. The non-blocking read
 * gets us out until split() is called.
 *
 * Some users (apparently just on 3B2 DKHOST systems) have had problems with the
 * two process implementation that's forced me to kludge things up some. When a
 * printer (on those systems) is turned off while postio is transmitting files
 * the write process hangs in writeblock() (postio.c) - it's typically in the
 * middle of a write() call, while the read() call (below) continually returns 0.
 * In the original code readline() returned FALSE when read() returned 0 and we
 * get into a loop that never ends - because the write process is hung. In the
 * one process implementation having read return 0 is legitimate because the line
 * is opened for no delay, but with two processes the read() blocks and a return
 * value of 0 should never occur. From my point of view the real problem is that
 * the write() call hangs on 3B2 DKHOST systems and apparently doesn't anywhere
 * else. If the write returned anything less than or equal to 0 writeblock() would
 * shut things down. The kludge I've implemented counts the number of consecutive
 * times read() returns a 0 and if it exceeds a limit (100) the read process will
 * shut things down. In fact one return of 0 from read() when we're in the two
 * process mode is undoubtedly sufficient and no counting should be necessary!!!
 * Moving the check to getstatus() should also work and is probably where things
 * belong.
 *
 */

    if ( interactive == FALSE )  {
	while ( (n = read(ttyi, ptr, 1)) != 0 )  {
	    if ( n < 0 )
		if ( errno == EINTR )
		    continue;
		else error(FATAL, "error reading %s", line);
	    tries = 0;
	    if ( *ptr == '\n' || *ptr == '\004' || ptr >= endmesg )  {
		*(ptr+1) = '\0';
		if ( *ptr == '\004' )
		    strcpy(ptr, "%%[ status: endofjob ]%%\n");
		ptr = mesg;
		return(TRUE);
	    }   /* End if */
	    ptr++;
	}   /* End while */
	if ( canread == TRUE && canwrite == FALSE )	/* read process kludge */
	    if ( ++tries > 100 )
		error(FATAL, "printer appears to be offline - shutting down");
	return(FALSE);
    }	/* End if */

    if ( canwrite == TRUE )		/* don't block during start() */
	return(FALSE);

    while ( (ch = getc(fp_ttyi)) != EOF )
	putc(ch, stdout);
    return(FALSE);

}   /* End of readline */
#endif

/*****************************************************************************/

#ifdef V9
#include <ipc.h>

char	tbuf[256];			/* temporary input buffer */
char	*nptr = tbuf;			/* next character comes from here */
char	*eptr = tbuf;			/* one past the last character in tbuf */

setupline()

{

    struct sgttyb	sgtty;
    struct ttydevb	ttydev;		/* for setting up the line */
    static struct tchars	tchar = { '\377',	/* interrupt */
					  '\377',	/* quit */
					  '\021',	/* start output */
					  '\023',	/* stop output */
					  '\377',	/* end-of-file */
					  '\377'	/* input delimiter */
					};

/*
 *
 * Line initialization for V9.
 *
 */

    if ( line == NULL )  {
	ttyi = ttyo = 1;
	return;
    }	/* End if */
    alarm(120);			/* watch for hanging opens */
    if ( line[0] == '/' ) {
	if ( (ttyi = open(line, O_RDWR)) == -1 )
	error(FATAL, "can't open %s", line);
    } else if ((ttyi = ipcopen(ipcpath(line, "dk", 0), "")) < 0) {
		sleep(5);	/* wait for Datakit to hangup */
		if ((ttyi = ipcopen(ipcpath(line, "dk", 0), "")) < 0) {
			fprintf(stderr, "%s", errstr);
			error(FATAL, "can't ipcopen %s", line);
		}
    }
    alarm(0);

    if ( (ttyo = dup(ttyi)) == -1 )
	error(FATAL, "can't dup file descriptor for %s", line);

    if ( ioctl(ttyi, FIOPUSHLD, &tty_ld) == -1 )
	error(FATAL, "ioctl error - FIOPUSHLD");

    if ( ioctl(ttyi, TIOCGDEV, &ttydev) == -1 )
	error(FATAL, "ioctl error - TIOCGDEV");

    if ( ioctl(ttyi, TIOCGETP, &sgtty) == -1 )
	error(FATAL, "ioctl error - TIOCGETP");

    sgtty.sg_flags &= ~ECHO;
    sgtty.sg_flags &= ~CRMOD;
    sgtty.sg_flags |= CBREAK;
    ttydev.ispeed = baudrate;
    ttydev.ospeed = baudrate;

    if ( ioctl(ttyi, TIOCSDEV, &ttydev) == -1 )
	error(FATAL, "ioctl error - TIOCSDEV");

    if ( ioctl(ttyi, TIOCSETP, &sgtty) == -1 )
	error(FATAL, "ioctl error - TIOCSETP");

    if ( ioctl(ttyi, TIOCSETC, &tchar) == -1 )
	error(FATAL, "ioctl error - TIOCSETC");

    fp_ttyi = fdopen(ttyi, "r");

}   /* End of setupline */

/*****************************************************************************/

resetline()

{

    struct sgttyb	sgtty;

/*
 *
 * Only used if we're running the program as separate read and write processes.
 * Called from split() after the initial connection has been made and returns
 * TRUE if two processes should work. Haven't tested or even compiled the stuff
 * for separate read and write processes on Ninth Edition systems - no guarantees
 * even though we return TRUE!
 *
 */

    if ( ioctl(ttyi, TIOCGETP, &sgtty) == -1 )
	error(FATAL, "ioctl error - TIOCGETP");

    sgtty.sg_flags |= TANDEM;

    if ( ioctl(ttyi, TIOCSETP, &sgtty) == -1 )
	error(FATAL, "ioctl error - TIOCSETP");

    return(TRUE);

}   /* End of resetline */

/*****************************************************************************/

setupstdin(mode)

    int		mode;			/* what to do with stdin settings */

{

    struct sgttyb		sgtty;

    static int			saved = FALSE;
    static struct sgttyb	oldsgtty;

/*
 *
 * Save (mode = 0), reset (mode = 1), or restore (mode = 2) the tty settings for
 * stdin. Expect something like raw mode with no echo will be set up. Need to make
 * sure interrupt and quit still work - they're the only good way to exit when
 * we're running interactive mode. I haven't tested or even compiled this code
 * so there are no guarantees.
 *
 */

    if ( interactive == TRUE )
	switch ( mode )  {
	    case 0:
		if ( ioctl(0, TIOCGETP, &oldsgtty) == -1 )
		    error(FATAL, "can't save terminal settings");
		saved = TRUE;
		break;

	    case 1:
		sgtty = oldsgtty;
		sgtty.sg_flags &= ~ECHO;
		sgtty.sg_flags |= CBREAK;
		ioctl(0, TIOCSETP, &sgtty);
		break;

	    case 2:
		if ( saved == TRUE )
		    ioctl(0, TIOCSETP, &oldsgtty);
		break;
	}   /* End switch */

}   /* End of setupstdin */

/*****************************************************************************/

readline()

{

    int		n;			/* read() return value */
    int		ch;			/* for interactive mode */

/*
 *
 * Reads characters coming back from the printer on ttyi up to a newline (or EOF)
 * and transfers each line to the mesg[] array. Everything available on ttyi is
 * initially stored in tbuf[] and a line at a time is transferred from there to
 * mesg[]. The string in mesg[] is terminated with a '\0' and TRUE is returned to
 * the caller when we find a newline, EOF, or reach the end of the mesg[] array.
 * If nothing is available on ttyi we return FALSE if a single process is being
 * used for reads and writes, while in the two process implementation we force a
 * one character read. Interactive mode loops here forever, except during start(),
 * echoing everything that comes back on ttyi to stdout. The performance of a
 * simple getc/putc loop for interactive mode was unacceptable when run under mux
 * and has been replaced by more complicated code. When layers wasn't involved
 * the getc/putc loop worked well.
 *
 */

    if ( interactive == FALSE )  {
	while ( 1 )  {
	    while ( nptr < eptr )  {	/* grab characters from tbuf */
		*ptr = *nptr++;
		if ( *ptr == '\r' ) continue;
		if ( *ptr == '\n' || *ptr == '\004' || ptr >= endmesg )  {
		    *(ptr+1) = '\0';
		    if ( *ptr == '\004' )
			strcpy(ptr, "%%[ status: endofjob ]%%\n");
		    ptr = mesg;
		    return(TRUE);
		}   /* End if */
		++ptr;
	    }	/* End for */

	    nptr = eptr = tbuf;
	    if ( ioctl(ttyi, FIONREAD, &n) < 0 )
		if ( errno == EINTR )
		    continue;
		else error(FATAL, "ioctl error - FIONREAD");
	    if ( n <= 0 )
		if ( canwrite == TRUE )
		    return(FALSE);
	    n = ((n < 1) ? 1 : ((n < sizeof(tbuf)) ? n : sizeof(tbuf)));
	    if ( (n = read(ttyi, tbuf, n)) < 0 )
		if ( errno == EINTR )
		    continue;
		else error(FATAL, "error reading line %s", line);
	    else eptr = nptr + n;
	}   /* End while */
    }	/* End if */

    if ( canwrite == TRUE )		/* don't block during start() */
	return(FALSE);

    while ( 1 )  {			/* only interactive mode gets here */
	if ( ioctl(ttyi, FIONREAD, &n) < 0 )
	    error(FATAL, "ioctl error - FIONREAD");
	n = ((n < 1) ? 1 : ((n < sizeof(tbuf)) ? n : sizeof(tbuf)));
	if ( (n = read(ttyi, tbuf, n)) < 0 )
	    error(FATAL, "error reading line %s", line);
	else if ( n == 0 )		/* should not happen */
	    error(FATAL, "end of file in interactive mode");
	if ( write(1, tbuf, n) != n )
	    error(FATAL, "error writing to stdout");
    }	/* End while */

    return(FALSE);

}   /* End of readline */
#endif

/*****************************************************************************/

#ifdef BSD4_2
setupline()

{

    struct sgttyb	sgtty;
    static struct tchars	tchar = { '\377',	/* interrupt */
					  '\377',	/* quit */
					  '\021',	/* start output */
					  '\023',	/* stop output */
					  '\377',	/* end-of-file */
					  '\377'	/* input delimiter */
					};
    long	lmodes;
    int		disc = NTTYDISC;

/*
 *
 * Line initialization for BSD4_2. As in the System V code, if no line is given
 * (ie. line == NULL) we continue on as before using stdout as ttyi and ttyo.
 *
 */

    if ( line == NULL )
	ttyi = fileno(stdout);
    else if ( (ttyi = open(line, O_RDWR)) == -1 )
	error(FATAL, "can't open %s", line);

    if ( (ttyo = dup(ttyi)) == -1 )
	error(FATAL, "can't dup file descriptor for %s", line);

    if (ioctl(ttyi, TIOCSETD, &disc) == -1 )
	error(FATAL, "ioctl error - TIOCSETD");

    if ( ioctl(ttyi, TIOCGETP, &sgtty) == -1 )
	error(FATAL, "ioctl error - TIOCGETP");

    if ( ioctl(ttyi, TIOCLGET, &lmodes) == -1 )
	error(FATAL, "ioctl error - TIOCLGET");

    sgtty.sg_flags &= ~ECHO;
    sgtty.sg_flags &= ~CRMOD;
    sgtty.sg_flags |= CBREAK;
    sgtty.sg_ispeed = baudrate;
    sgtty.sg_ospeed = baudrate;
    lmodes |= LDECCTQ;

    if ( ioctl(ttyi, TIOCSETP, &sgtty) == -1 )
	error(FATAL, "ioctl error - TIOCSETP");

    if ( ioctl(ttyi, TIOCSETC, &tchar) == -1 )
	error(FATAL, "ioctl error - TIOCSETC");

    if ( ioctl(ttyi, TIOCLSET, &lmodes) == -1 )
	error(FATAL, "ioctl error - TIOCLSET");

    fp_ttyi = fdopen(ttyi, "r");

}   /* End of setupline */

/*****************************************************************************/

resetline()

{

    struct sgttyb	sgtty;

/*
 *
 * Only used if we're running the program as separate read and write processes.
 * Called from split() after the initial connection has been made and returns
 * TRUE if two processes should work. Haven't tested or even compiled the stuff
 * for separate read and write processes on Berkeley systems - no guarantees
 * even though we return TRUE!
 *
 */

    if ( ioctl(ttyi, TIOCGETP, &sgtty) == -1 )
	error(FATAL, "ioctl error - TIOCGETP");

    sgtty.sg_flags |= TANDEM;

    if ( ioctl(ttyi, TIOCSETP, &sgtty) == -1 )
	error(FATAL, "ioctl error - TIOCSETP");

    return(TRUE);

}   /* End of resetline */

/*****************************************************************************/

setupstdin(mode)

    int		mode;			/* what to do with stdin settings */

{

    struct sgttyb		sgtty;

    static int			saved = FALSE;
    static struct sgttyb	oldsgtty;

/*
 *
 * Save (mode = 0), reset (mode = 1), or restore (mode = 2) the tty settings for
 * stdin. Expect something like raw mode with no echo will be set up. Need to make
 * sure interrupt and quit still work - they're the only good way to exit when
 * we're running interactive mode. I haven't tested or even compiled this code
 * so there are no guarantees.
 *
 */

    if ( interactive == TRUE )
	switch ( mode )  {
	    case 0:
		if ( isatty(0) != 1 )
		    error(FATAL, "stdin not a terminal - can't run interactive mode");
		if ( ioctl(0, TIOCGETP, &oldsgtty) == -1 )
		    error(FATAL, "can't save terminal settings");
		saved = TRUE;
		break;

	    case 1:
		sgtty = oldsgtty;
		sgtty.sg_flags &= ~ECHO;
		sgtty.sg_flags |= CBREAK;
		ioctl(0, TIOCSETP, &sgtty);
		break;

	    case 2:
		if ( saved == TRUE )
		    ioctl(0, TIOCSETP, &oldsgtty);
		break;
	}   /* End switch */

}   /* End of setupstdin */

/*****************************************************************************/

readline()

{

    int		n;			/* read() return value */
    int		ch;			/* for interactive mode */

/*
 *
 * Reads characters coming back from the printer on ttyo up to a newline (or EOF)
 * or until no more characters are available. Characters are put in mesg[], the
 * string is terminated with '\0' when we're done with a line and TRUE is returned
 * to the caller. If complete line wasn't available FALSE is returned. Interactive
 * mode should loop here forever, except during start(), echoing characters to
 * stdout. If it happens to leave FALSE should be returned. Probably should read
 * everything available on ttyi into a temporary buffer and work from there rather
 * than reading one character at a time.
 *
 */

    if ( interactive == FALSE )  {
	while ( 1 )  {
	    if ( ioctl(ttyi, FIONREAD, &n) < 0 )
		if ( errno == EINTR )
		    continue;
		else error(FATAL, "ioctl error - FIONREAD");
	    if ( n <= 0 )
		if ( canwrite == TRUE )
		    return(FALSE);
		else n = 1;
	    for ( ; n > 0; n-- )  {
		/*if ( read(ttyi, ptr, 1) < 0 )*/
		if ( (*ptr = getc(fp_ttyi)) == EOF )
		    if ( errno == EINTR )
			continue;
		    else error(FATAL, "error reading %s", line);
		if ( *ptr == '\r' ) continue;
		if ( *ptr == '\n' || *ptr == '\004' || ptr >= endmesg )  {
		    *(ptr+1) = '\0';
		    if ( *ptr == '\004' )
			strcpy(ptr, "%%[ status: endofjob ]%%\n");
		    ptr = mesg;
		    return(TRUE);
		}   /* End if */
		++ptr;
	    }	/* End for */
	}   /* End while */
    }	/* End if */

    if ( canwrite == TRUE )		/* don't block during start() */
	return(FALSE);

    while ( (ch = getc(fp_ttyi)) != EOF )
	putc(ch, stdout);
    return(FALSE);

}   /* End of readline */

/*****************************************************************************/

/*	@(#)strspn.c	1.2	*/
/*LINTLIBRARY*/
/*
 * Return the number of characters in the maximum leading segment
 * of string which consists solely of characters from charset.
 */
int
strspn(string, charset)
int8_t	*string;
register char	*charset;
{
	register char *p, *q;

	for(q=string; *q != '\0'; ++q) {
		for(p=charset; *p != '\0' && *p != *q; ++p)
			;
		if(*p == '\0')
			break;
	}
	return(q-string);
}

/*	@(#)strpbrk.c	1.2	*/
/*LINTLIBRARY*/
/*
 * Return ptr to first occurance of any character from `brkset'
 * in the character string `string'; NULL if none exists.
 */

char *
strpbrk(string, brkset)
register char *string, *brkset;
{
	register char *p;

	do {
		for(p=brkset; *p != '\0' && *p != *string; ++p)
			;
		if(*p != '\0')
			return(string);
	}
	while(*string++);
	return((char*)0);
}

/*	@(#)strtok.c	1.2	*/
/*	3.0 SID #	1.2	*/
/*LINTLIBRARY*/
/*
 * uses strpbrk and strspn to break string into tokens on
 * sequentially subsequent calls.  returns NULL when no
 * non-separator characters remain.
 * `subsequent' calls are calls with first argument NULL.
 */


extern int strspn();
extern char *strpbrk();

char *
strtok(string, sepset)
int8_t	*string, *sepset;
{
	register char	*p, *q, *r;
	static char	*savept;

	/*first or subsequent call*/
	p = (string == (char*)0)? savept: string;

	if(p == 0)		/* return if no tokens remaining */
		return((char*)0);

	q = p + strspn(p, sepset);	/* skip leading separators */

	if(*q == '\0')		/* return if no tokens remaining */
		return((char*)0);

	if((r = strpbrk(q, sepset)) == (char*)0)	/* move past token */
		savept = 0;	/* indicate this is last token */
	else {
		*r = '\0';
		savept = ++r;
	}
	return(q);
}
#endif

/*****************************************************************************/

#ifdef DKHOST

#ifndef DKSTREAMS
int16_t	dkrmode[3] = {DKR_TIME, 0, 0};
#endif

dkhost_connect()

{

    int		ofd;			/* for saving and restoring stderr */
    int		dfd;
    int		retrytime = 5;

/*
 *
 * Tries to connect to a Datakit destination. The extra stuff I've added to save
 * and later restore stderr is primarily for our spooling setup at Murray Hill.
 * postio is usually called with stderr directed to a file that will be returned
 * to the user when the job finishes printing. Problems encountered by dkdial(),
 * like busy messages, go to stderr but don't belong in the user's mail. They'll
 * be temporarily directed to the log file. After we've connected stderr will be
 * restored.
 *
 */

    if ( *line == '\0' )
	error(FATAL, "incomplete Datakit line");

    if ( fp_log != NULL && fp_log != stderr )  {	/* redirect dkdial errors */
	ofd = dup(2);
	close(2);
	dup(fileno(fp_log));
    }	/* End if */

    while ( (dfd = ttyi = dkdial(line)) < 0 )  {
	if ( retrytime < 0 )
	    error(FATAL, "can't connect to %s", line);
	sleep(retrytime++);
	if ( retrytime > 60 )
	    retrytime = 60;
    }	/* End while */

    if ( fp_log != NULL && fp_log != stderr )  {	/* restore stderr */
	close(2);
	dup(ofd);
	close(ofd);
    }	/* End if */

#ifndef DKSTREAMS
    if ( ioctl(ttyi, DIOCRMODE, dkrmode) == -1 )
	error(FATAL, "ioctl error - DIOCRMODE");

#ifdef DIOURPWD
    if ( window_size > 0 ) {
	short	dkparm[3];

	dkparm[0] = dkminor(ttyi);
	dkparm[1] = 1;
	dkparm[2] = window_size;
	if ( ioctl(ttyi, DIOURPWD, dkparm) < 0 || ioctl(ttyi, DIOCFLUSH, 0) < 0 )
	    error(NON_FATAL, "WSA failed");
    }	/* End if */
#endif

    line = dtnamer(dkminor(ttyi));

    if ( (ttyi = open(line, O_RDWR)) == -1 )
	error(FATAL, "can't open %s", line);

    close(dfd);
#endif

}   /* End of dkhost_connect */
#endif

/*****************************************************************************/
