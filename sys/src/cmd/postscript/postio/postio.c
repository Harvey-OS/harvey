/*
 *
 * postio - RS-232 serial interface for PostScript printers
 *
 * A simple program that manages input and output for PostScript printers. Much
 * has been added and changed from early versions of the program, but the basic
 * philosophy is still the same. Don't send real data until we're certain we've
 * connected to a PostScript printer that's in the idle state and try to hold the
 * connection until the job is completely done. It's more work than you might
 * expect is necessary, but should provide a reasonably reliable spooler interface
 * that can return error indications to the caller via the program's exit status.
 *
 * I've added code that will let you split the program into separate read/write
 * processes. Although it's not the default it should be useful if you have a file
 * that will be returning useful data from the printer. The two process stuff was
 * laid down on top of the single process code and both methods still work. The
 * implementation isn't as good as it could be, but didn't require many changes
 * to the original program (despite the fact that there are now many differences).
 *
 * By default the program still runs as a single process. The -R2 option forces
 * separate read and write processes after the intial connection is made. If you
 * want that as the default initialize splitme (below) to TRUE. In addition the
 * -t option that's used to force stuff not recognized as status reports to stdout
 * also tries to run as two processes (by setting splitme to TRUE). It will only
 * work if the required code (ie. resetline() in ifdef.c) has been implemented
 * for your Unix system. I've only tested the System V code.
 *
 * Code needed to support interactive mode has also been added, although again it's
 * not as efficient as it could be. It depends on the system dependent procedures
 * resetline() and setupstdin() (file ifdef.c) and for now is only guaranteed to
 * work on System V. Can be requested using the -i option.
 *
 * Quiet mode (-q option) is also new, but was needed for some printers connected
 * to RADIAN. If you're running in quiet mode no status requests will be sent to
 * the printer while files are being transmitted (ie. in send()).
 *
 * The program expects to receive printer status lines that look like,
 *
 *	%%[ status: idle; source: serial 25 ]%%
 *	%%[ status: waiting; source: serial 25 ]%%
 *	%%[ status: initializing; source: serial 25 ]%%
 *	%%[ status: busy; source: serial 25 ]%%
 *	%%[ status: printing; source: serial 25 ]%%
 *	%%[ status: PrinterError: out of paper; source: serial 25 ]%%
 *	%%[ status: PrinterError: no paper tray; source: serial 25 ]%%
 *
 * although this list isn't complete. Sending a '\024' (control T) character forces
 * the return of a status report. PostScript errors detected on the printer result
 * in the immediate transmission of special error messages that look like,
 *
 *	%%[ Error: undefined; OffendingCommand: xxx ]%%
 *	%%[ Flushing: rest of job (to end-of-file) will be ignored ]%%
 *
 * although we only use the Error and Flushing keywords. Finally conditions, like
 * being out of paper, result in other messages being sent back from the printer
 * over the communications line. Typical PrinterError messages look like,
 *
 *	%%[ PrinterError: out of paper; source: serial 25 ]%%
 *	%%[ PrinterError: paper jam; source: serial 25 ]%%
 *
 * although we only use the PrinterError keyword rather than trying to recognize
 * all possible printer errors.
 *
 * The implications of using one process and only flow controlling data going to
 * the printer are obvious. Job transmission should be reliable, but there can be
 * data loss in stuff sent back from the printer. Usually that only caused problems
 * with jobs designed to run on the printer and return useful data back over the
 * communications line. If that's the kind of job you're sending call postio with
 * the -t option. That should force the program to split into separate read and
 * write processes and everything not bracketed by "%%[ " and " ]%%" strings goes
 * to stdout. In otherwords the data you're expecting should be separated from the
 * status stuff that goes to the log file (or stderr). The -R2 option does almost
 * the same thing (ie. separate read and write processes), but everything that
 * comes back from the printer goes to the log file (stderr by default) and you'll
 * have to separate your data from any printer messages.
 *
 * A typical command line might be,
 *
 *	postio -l /dev/tty01 -b 9600 -L log file1 file2
 *
 * where -l selects the line, -b sets the baud rate, and -L selects the printer
 * log file. Since there's no default line, at least not right now, you'll always
 * need to use the -l option, and if you don't choose a log file stderr will be
 * used. If you have a program that will be returning data the command line might
 * look like,
 *
 *	postio -t -l/dev/tty01 -b9600 -Llog file >results
 *
 * Status stuff goes to file log while the data you're expecting back from the
 * printer gets put in file results.
 *
 */

#include <stdio.h>
#include <ctype.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <errno.h>

#include "ifdef.h"			/* conditional compilation stuff */
#include "gen.h"			/* general purpose definitions */
#include "postio.h"			/* some special definitions */

char	**argv;				/* global so everyone can use them */
int	argc;

char	*prog_name = "";		/* really just for error messages */
int	x_stat = 0;			/* program exit status */
int	debug = OFF;			/* debug flag */
int	ignore = OFF;			/* what's done for FATAL errors */

char	*line = NULL;			/* printer is on this tty line */
short	baudrate = BAUDRATE;		/* and running at this baud rate */
Baud	baudtable[] = BAUDTABLE;	/* converts strings to termio values */

int	stopbits = 1;			/* number of stop bits */
int	tostdout = FALSE;		/* non-status stuff goes to stdout? */
int	quiet = FALSE;			/* no status queries in send() if TRUE */
int	interactive = FALSE;		/* interactive mode */
char	*postbegin = POSTBEGIN;		/* preceeds all the input files */
int	useslowsend = FALSE;		/* not recommended! */
int	sendctrlC = TRUE;		/* interrupt with ctrl-C when BUSY */
int	window_size = -1;		/* for Datakit - use -w */

char	*block = NULL;			/* input file buffer */
int	blocksize = BLOCKSIZE;		/* and its size in bytes */
int	head = 0;			/* block[head] is the next character */
int	tail = 0;			/* one past the last byte in block[] */

int	splitme = FALSE;		/* into READ and WRITE processes if TRUE */
int	whatami = READWRITE;		/* a READ or WRITE process - or both */
int	canread = TRUE;			/* allow reads */
int	canwrite = TRUE;		/* and writes if TRUE */
int	otherpid = -1;			/* who gets signals if greater than 1 */
int	joinsig = SIGTRAP;		/* reader gets this when writing is done */
int	writedone = FALSE;		/* and then sets this to TRUE */

char	mesg[MESGSIZE];			/* exactly what came back on ttyi */
char	sbuf[MESGSIZE];			/* for parsing the message */
int	next = 0;			/* next character goes in mesg[next] */
char	*mesgptr = NULL;		/* printer message starts here in mesg[] */
char	*endmesg = NULL;		/* as far as readline() can go in mesg[] */

Status	status[] = STATUS;		/* for converting status strings */
int	nostatus = NOSTATUS;		/* default getstatus() return value */

int	currentstate = NOTCONNECTED;	/* what's happening START, SEND, or DONE */

int	ttyi = 0;			/* input */
int	ttyo = 2;			/* and output file descriptors */

FILE	*fp_log = stderr;		/* log file for stuff from the printer */

/*****************************************************************************/

main(agc, agv)

    int		agc;
    char	*agv[];

{

/*
 *
 * A simple program that manages input and output for PostScript printers. Can run
 * as a single process or as separate read/write processes. What's done depends on
 * the value assigned to splitme when split() is called.
 *
 */

    argc = agc;				/* other routines may want them */
    argv = agv;

    prog_name = argv[0];		/* really just for error messages */

    init_signals();			/* sets up interrupt handling */
    options();				/* get command line options */
    initialize();			/* must be done after options() */
    start();				/* make sure the printer is ready */
    split();				/* into read/write processes - maybe */
    arguments();			/* then send each input file */
    done();				/* wait until the printer is finished */
    cleanup();				/* make sure the write process stops */

    exit(x_stat);			/* everything probably went OK */

}   /* End of main */

/*****************************************************************************/

init_signals()

{

    void	interrupt();		/* handles them if we catch signals */

/*
 *
 * Makes sure we handle interrupts. The proper way to kill the program, if
 * necessary, is to do a kill -15. That forces a call to interrupt(), which in
 * turn tries to reset the printer and then exits with a non-zero status. If the
 * program is running as two processes, sending SIGTERM to either the parent or
 * child should clean things up.
 *
 */

    if ( signal(SIGINT, interrupt) == SIG_IGN )  {
	signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	signal(SIGHUP, SIG_IGN);
    } else {
	signal(SIGHUP, interrupt);
	signal(SIGQUIT, interrupt);
    }	/* End else */

    signal(SIGTERM, interrupt);

}   /* End of init_sig */

/*****************************************************************************/

options()

{

    int		ch;			/* return value from getopt() */
    char	*optnames = "b:cil:qs:tw:B:L:P:R:SDI";

    extern char	*optarg;		/* used by getopt() */
    extern int	optind;

/*
 *
 * Reads and processes the command line options. The -R2, -t, and -i options all
 * force separate read and write processes by eventually setting splitme to TRUE
 * (check initialize()). The -S option is not recommended and should only be used
 * as a last resort!
 *
 */

    while ( (ch = getopt(argc, argv, optnames)) != EOF )  {
	switch ( ch )  {
	    case 'b':			/* baud rate string */
		    baudrate = getbaud(optarg);
		    break;

	    case 'c':			/* no ctrl-C's */
		    sendctrlC = FALSE;
		    break;

	    case 'i':			/* interactive mode */
		    interactive = TRUE;
		    break;

	    case 'l':			/* printer line */
		    line = optarg;
		    break;

	    case 'q':			/* no status queries - for RADIAN? */
		    quiet = TRUE;
		    break;

	    case 's':			/* use 2 stop bits - for UNISON? */
		    if ( (stopbits = atoi(optarg)) < 1 || stopbits > 2 )
			stopbits = 1;
		    break;

	    case 't':			/* non-status stuff goes to stdout */
		    tostdout = TRUE;
		    break;

	    case 'w':			/* Datakit window size */
		    window_size = atoi(optarg);
		    break;

	    case 'B':			/* set the job buffer size */
		    if ( (blocksize = atoi(optarg)) <= 0 )
			blocksize = BLOCKSIZE;
		    break;

	    case 'L':			/* printer log file */
		    if ( (fp_log = fopen(optarg, "w")) == NULL )  {
			fp_log = stderr;
			error(NON_FATAL, "can't open log file %s", optarg);
		    }	/* End if */
		    break;

	    case 'P':			/* initial PostScript code */
		    postbegin = optarg;
		    break;

	    case 'R':			/* run as one or two processes */
		    if ( atoi(optarg) == 2 )
			splitme = TRUE;
		    else splitme = FALSE;
		    break;

	    case 'S':			/* slow and kludged up version of send */
		    useslowsend = TRUE;
		    break;

	    case 'D':			/* debug flag */
		    debug = ON;
		    break;

	    case 'I':			/* ignore FATAL errors */
		    ignore = ON;
		    break;

	    case '?':			/* don't understand the option */
		    error(FATAL, "");
		    break;

	    default:			/* don't know what to do for ch */
		    error(FATAL, "missing case for option %c\n", ch);
		    break;
	}   /* End switch */
    }   /* End while */

    argc -= optind;			/* get ready for non-option args */
    argv += optind;

}   /* End of options */

/*****************************************************************************/

getbaud(rate)

    char	*rate;			/* string representing the baud rate */

{

    int		i;			/* for looking through baudtable[] */

/*
 *
 * Called from options() to convert a baud rate string into an appropriate termio
 * value. *rate is looked up in baudtable[] and if it's found, the corresponding
 * value is returned to the caller.
 *
 */

    for ( i = 0; baudtable[i].rate != NULL; i++ )
	if ( strcmp(rate, baudtable[i].rate) == 0 )
	    return(baudtable[i].val);

    error(FATAL, "don't recognize baud rate %s", rate);

}   /* End of getbaud */

/*****************************************************************************/

initialize()

{

/*
 *
 * Initialization, a few checks, and a call to setupline() (file ifdef.c) to open
 * and configure the communications line. Settings for interactive mode always
 * take precedence. The setupstdin() call with an argument of 0 saves the current
 * terminal settings if interactive mode has been requested - otherwise nothing's
 * done. Unbuffering stdout (via the setbuf() call) isn't really needed on System V
 * since it's flushed whenever terminal input is requested. It's more efficient if
 * we buffer the stdout (on System V) but safer (for other versions of Unix) if we
 * include the setbuf() call.
 *
 */

    whatami = READWRITE;		/* always run start() as one process */
    canread = canwrite = TRUE;

    if ( tostdout == TRUE )		/* force separate read/write processes */
	splitme = TRUE;

    if ( interactive == TRUE )  {	/* interactive mode settings always win */
	quiet = FALSE;
	tostdout = FALSE;
	splitme = TRUE;
	blocksize = 1;
	postbegin = NULL;
	useslowsend = FALSE;
	nostatus = INTERACTIVE;
	setbuf(stdout, NULL);
    }	/* End if */

    if ( useslowsend == TRUE )  {	/* last resort only - not recommended */
	quiet = FALSE;
	splitme = FALSE;
	if ( blocksize > 1024 )		/* don't send too much all at once */
	    blocksize = 1024;
    }	/* End if */

    if ( tostdout == TRUE && fp_log == stderr )
	fp_log = NULL;

    if ( line == NULL && (interactive == TRUE || tostdout == TRUE) )
	error(FATAL, "a printer line must be supplied - use the -l option");

    if ( (block = malloc(blocksize)) == NULL )
	error(FATAL, "no memory");

    endmesg = mesg + sizeof mesg - 2;	/* one byte from last position in mesg */

    setupline();			/* configure the communications line */
    setupstdin(0);			/* save current stdin terminal settings */

}   /* End of initialize */

/*****************************************************************************/

start()

{

/*
 *
 * Tries to put the printer in the IDLE state before anything important is sent.
 * Run as a single process no matter what has been assigned to splitme. Separate
 * read and write processes, if requested, will be created after we're done here.
 *
 */

    logit("printer startup\n");

    currentstate = START;
    clearline();

    while ( 1 )
	switch ( getstatus(1) )  {
	    case IDLE:
	    case INTERACTIVE:
		    if ( postbegin != NULL && *postbegin != '\0' )
			Write(ttyo, postbegin, strlen(postbegin));
		    clearline();
		    return;

	    case BUSY:
		    if ( sendctrlC == TRUE ) {
			Write(ttyo, "\003", 1);
			Rest(1);
		    }	/* End if */
		    break;

	    case WAITING:
	    case ERROR:
	    case FLUSHING:
		    Write(ttyo, "\004", 1);
		    Rest(1);
		    break;

	    case PRINTERERROR:
		    Rest(15);
		    break;

	    case DISCONNECT:
		    error(FATAL, "Disconnected - printer may be offline");
		    break;

	    case ENDOFJOB:
	    case UNKNOWN:
		    clearline();
		    break;

	    default:
		    Rest(1);
		    break;
	}   /* End switch */

}   /* End of start */

/*****************************************************************************/

split()

{

    int		pid;
    void	interrupt();

/*
 *
 * If splitme is TRUE we fork a process, make the parent handle reading, and let
 * the child take care of writing. resetline() (file ifdef.c) contains all the
 * system dependent code needed to reset the communications line for separate
 * read and write processes. For now it's expected to return TRUE or FALSE and
 * that value controls whether we try the fork. I've only tested the two process
 * stuff for System V. Other versions of resetline() may just be dummy procedures
 * that always return FALSE. If the fork() failed previous versions continued as
 * a single process, although the implementation wasn't quite right, but I've now
 * decided to quit. The main reason is a Datakit channel may be configured to
 * flow control data in both directions, and if we run postio over that channel
 * as a single process we likely will end up in deadlock.
 *
 */

    if ( splitme == TRUE )
	if ( resetline() == TRUE )  {
	    pid = getpid();
	    signal(joinsig, interrupt);
	    if ( (otherpid = fork()) == -1 )
		error(FATAL, "can't fork");
	    else if ( otherpid == 0 )  {
		whatami = WRITE;
		nostatus = WRITEPROCESS;
		otherpid = pid;
		setupstdin(1);
	    } else whatami = READ;
	} else if ( interactive == TRUE || tostdout == TRUE )
	    error(FATAL, "can't create two process - check resetline()");
 	else error(NON_FATAL, "running as a single process - check resetline()");

    canread = (whatami & READ) ? TRUE : FALSE;
    canwrite = (whatami & WRITE) ? TRUE : FALSE;

}   /* End of split */

/*****************************************************************************/

arguments()

{

    int		fd_in;			/* next input file */

/*
 *
 * Makes sure all the non-option command line arguments are processed. If there
 * aren't any arguments left when we get here we'll send stdin. Input files are
 * only read and sent to the printer if canwrite is TRUE. Checking it here means
 * we won't have to do it in send(). If interactive mode is TRUE we'll stay here
 * forever sending stdin when we run out of files - exit with a break. Actually
 * the loop is bogus and used at most once when we're in interactive mode because
 * stdin is in a pseudo raw mode and the read() in readblock() should never see
 * the end of file.
 *
 */

    if ( canwrite == TRUE )
	do				/* loop is for interactive mode */
	    if ( argc < 1 )
		send(fileno(stdin), "pipe.end");
	    else  {
		while ( argc > 0 )  {
		    if ( (fd_in = open(*argv, O_RDONLY)) == -1 )
			error(FATAL, "can't open %s", *argv);
		    send(fd_in, *argv);
		    close(fd_in);
		    argc--;
		    argv++;
		}   /* End while */
	    }	/* End else */
	while ( interactive == TRUE );

}   /* End of arguments */

/*****************************************************************************/

send(fd_in, name)

    int		fd_in;			/* next input file */
    char	*name;			/* and it's pathname */

{

/*
 *
 * Sends file *name to the printer. There's nothing left here that depends on
 * sending and receiving status reports, although it can be reassuring to know
 * the printer is responding and processing our job. Only the writer gets here
 * in the two process implementation, and in that case split() has reset nostatus
 * to WRITEPROCESS and that's what getstatus() always returns. For now we accept
 * the IDLE state and ENDOFJOB as legitimate and ignore the INITIALIZING state.
 *
 */

    if ( interactive == FALSE )
	logit("sending file %s\n", name);

    currentstate = SEND;

    if ( useslowsend == TRUE )  {
	slowsend(fd_in);
	return;
    }	/* End if */

    while ( readblock(fd_in) )
	switch ( getstatus(0) )  {
	    case IDLE:
	    case BUSY:
	    case WAITING:
	    case PRINTING:
	    case ENDOFJOB:
	    case PRINTERERROR:
	    case UNKNOWN:
	    case NOSTATUS:
	    case WRITEPROCESS:
	    case INTERACTIVE:
		    writeblock();
		    break;

	    case ERROR:
		    fprintf(stderr, "%s", mesg);	/* for csw */
		    error(USER_FATAL, "PostScript Error");
		    break;

	    case FLUSHING:
		    error(USER_FATAL, "Flushing Job");
		    break;

	    case DISCONNECT:
		    error(FATAL, "Disconnected - printer may be offline");
		    break;
	}   /* End switch */

}   /* End of send */

/*****************************************************************************/

done()

{

    int		sleeptime = 15;		/* for 'out of paper' etc. */

/*
 *
 * Tries to stay connected to the printer until we're reasonably sure the job is
 * complete. It's the only way we can recover error messages or data generated by
 * the PostScript program and returned over the communication line. Actually doing
 * it correctly for all possible PostScript jobs is more difficult that it might
 * seem. For example if we've sent several jobs, each with their own EOF mark, then
 * waiting for ENDOFJOB won't guarantee all the jobs have completed. Even waiting
 * for IDLE isn't good enough. Checking for the WAITING state after all the files
 * have been sent and then sending an EOF may be the best approach, but even that
 * won't work all the time - we could miss it or might not get there. Even sending
 * our own special PostScript job after all the input files has it's own different
 * set of problems, but probably could work (perhaps by printing a fake status
 * message or just not timing out). Anyway it's probably not worth the trouble so
 * for now we'll quit if writedone is TRUE and we get ENDOFJOB or IDLE.
 *
 * If we're running separate read and write processes the reader gets here after
 * after split() while the writer goes to send() and only gets here after all the
 * input files have been transmitted. When they're both here the writer sends the
 * reader signal joinsig and that forces writedone to TRUE in the reader. At that
 * point the reader can begin looking for an indication of the end of the job.
 * The writer hangs around until the reader kills it (usually in cleanup()) sending
 * occasional status requests.
 *
 */

    if ( canwrite == TRUE )
	logit("waiting for end of job\n");

    currentstate = DONE;
    writedone = (whatami == READWRITE) ? TRUE : FALSE;

    while ( 1 )  {
	switch ( getstatus(1) )  {

	    case WRITEPROCESS:
		    if ( writedone == FALSE )  {
			sendsignal(joinsig);
			Write(ttyo, "\004", 1);
			writedone = TRUE;
			sleeptime = 1;
		    }	/* End if */
		    Rest(sleeptime++);
		    break;

	    case WAITING:
		    Write(ttyo, "\004", 1);
		    Rest(1);
		    sleeptime = 15;
		    break;

	    case IDLE:
	    case ENDOFJOB:
		    if ( writedone == TRUE )  {
			logit("job complete\n");
			return;
		    }	/* End if */
		    break;

	    case BUSY:
	    case PRINTING:
	    case INTERACTIVE:
		    sleeptime = 15;
		    break;

	    case PRINTERERROR:
		    Rest(sleeptime++);
		    break;

	    case ERROR:
		    fprintf(stderr, "%s", mesg);	/* for csw */
		    error(USER_FATAL, "PostScript Error");
		    return;

	    case FLUSHING:
		    error(USER_FATAL, "Flushing Job");
		    return;

	    case DISCONNECT:
		    error(FATAL, "Disconnected - printer may be offline");
		    return;

	    default:
		    Rest(1);
		    break;
	}   /* End switch */

	if ( sleeptime > 60 )
	    sleeptime = 60;
    }	/* End while */

}   /* End of done */

/*****************************************************************************/

cleanup()

{

    int		w;

/*
 *
 * Only needed if we're running separate read and write processes. Makes sure the
 * write process is killed after the read process has successfully finished with
 * all the jobs. sendsignal() returns a -1 if there's nobody to signal so things
 * work when we're running a single process.
 *
 */

    while ( sendsignal(SIGKILL) != -1 && (w = wait((int *)0)) != otherpid && w != -1 ) ;

}   /* End of cleanup */

/*****************************************************************************/

readblock(fd_in)

    int		fd_in;			/* current input file */

{

    static long	blocknum = 1;

/*
 *
 * Fills the input buffer with the next block, provided we're all done with the
 * last one. Blocks from fd_in are stored in array block[]. head is the index
 * of the next byte in block[] that's supposed to go to the printer. tail points
 * one past the last byte in the current block. head is adjusted in writeblock()
 * after each successful write, while head and tail are reset here each time
 * a new block is read. Returns the number of bytes left in the current block.
 * Read errors cause the program to abort. The fake status message that's put out
 * in quiet mode is only so you can look at the log file and know something's
 * happening - take it out if you want.
 *
 */

    if ( head >= tail )  {		/* done with the last block */
	if ( (tail = read(fd_in, block, blocksize)) == -1 )
	    error(FATAL, "error reading input file");
	if ( quiet == TRUE && tail > 0 )	/* put out a fake message? */
	    logit("%%%%[ status: busy; block: %d ]%%%%\n", blocknum++);
	head = 0;
    }	/* End if */

    return(tail - head);

}   /* End of readblock */

/*****************************************************************************/

writeblock()

{

    int		count;			/* bytes successfully written */

/*
 *
 * Called from send() when it's OK to send the next block to the printer. head
 * is adjusted after the write, and the number of bytes that were successfully
 * written is returned to the caller.
 *
 */

    if ( (count = write(ttyo, &block[head], tail - head)) == -1 )
	error(FATAL, "error writing to %s", line);
    else if ( count == 0 )
	error(FATAL, "printer appears to be offline");

    head += count;
    return(count);

}   /* End of writeblock */

/*****************************************************************************/

getstatus(t)

    int		t;			/* sleep time after sending '\024' */

{

    int		gotline = FALSE;	/* value returned by readline() */
    int		state = nostatus;	/* representation of the current state */
    int		mesgch;			/* to restore mesg[] when tostdout == TRUE */

    static int	laststate = NOSTATUS;	/* last state recognized */

/*
 *
 * Looks for things coming back from the printer on the communications line, parses
 * complete lines retrieved by readline(), and returns an integer representation
 * of the current printer status to the caller. If nothing was available a status
 * request (control T) is sent to the printer and nostatus is returned to the
 * caller (provided quiet isn't TRUE). Interactive mode either never returns from
 * readline() or returns FALSE.
 * 
 */

    if ( canread == TRUE && (gotline = readline()) == TRUE )  {
	state = parsemesg();
	if ( state != laststate || state == UNKNOWN || mesgptr != mesg || debug == ON )
	    logit("%s", mesg);

	if ( tostdout == TRUE && currentstate != START )  {
	    mesgch = *mesgptr;
	    *mesgptr = '\0';
	    fprintf(stdout, "%s", mesg);
	    fflush(stdout);
	    *mesgptr = mesgch;		/* for ERROR in send() and done() */
	}   /* End if */
	return(laststate = state);
    }	/* End if */

    if ( (quiet == FALSE || currentstate != SEND) &&
	 (tostdout == FALSE || currentstate == START) && interactive == FALSE )  {
	if ( Write(ttyo, "\024", 1) != 1 )
	    error(FATAL, "printer appears to be offline");
	if ( t > 0 ) Rest(t);
    }	/* End if */

    return(nostatus);

}   /* End of getstatus */

/*****************************************************************************/

parsemesg()

{

    char	*e;			/* end of printer message in mesg[] */
    char	*key, *val;		/* keyword/value strings in sbuf[] */
    char	*p;			/* for converting to lower case etc. */
    int		i;			/* where *key was found in status[] */

/*
 *
 * Parsing the lines that readline() stores in mesg[] is messy, and what's done
 * here isn't completely correct nor as fast as it could be. The general format
 * of lines that come back from the printer (assuming no data loss) is:
 *
 *		str%%[ key: val; key: val; key: val ]%%\n
 *
 * where str can be most anything not containing a newline and printer reports
 * (eg. status or error messages) are bracketed by "%%[ " and " ]%%" strings and
 * end with a newline. Usually we'll have the string or printer report but not
 * both. For most jobs the leading string will be empty, but could be anything
 * generated on a printer and returned over the communications line using the
 * PostScript print operator. I'll assume PostScript jobs are well behaved and
 * never bracket their messages with "%%[ " and " ]%%" strings that delimit status
 * or error messages.
 *
 * Printer reports consist of one or more key/val pairs, and what we're interested
 * in (status or error indications) may not be the first pair in the list. In
 * addition we'll sometimes want the value associated with a keyword (eg. when
 * key = status) and other times we'll want the keyword (eg. when key = Error or
 * Flushing). The last pair isn't terminated by a semicolon and a value string
 * often contains many space separated words and it can even include colons in
 * meaningful places. I've also decided to continue converting things to lower
 * case before doing the lookup in status[]. The isupper() test is for Berkeley
 * systems.
 *
 */

    if ( *(mesgptr = find("%%[ ", mesg)) != '\0' && *(e = find(" ]%%", mesgptr+4)) != '\0' )  {
	strcpy(sbuf, mesgptr+4);		/* don't change mesg[] */
	sbuf[e-mesgptr-4] = '\0';		/* ignore the trailing " ]%%" */

	for ( key = strtok(sbuf, " :"); key != NULL; key = strtok(NULL, " :") )  {
	    if ( (val = strtok(NULL, ";")) != NULL && strcmp(key, "status") == 0 )
		key = val;

	    for ( ; *key == ' '; key++ ) ;	/* skip any leading spaces */
	    for ( p = key; *p; p++ )		/* convert to lower case */
		if ( *p == ':' )  {
		    *p = '\0';
		    break;
		} else if ( isupper(*p) ) *p = tolower(*p);

	    for ( i = 0; status[i].state != NULL; i++ )
		if ( strcmp(status[i].state, key) == 0 )
		    return(status[i].val);
	}   /* End for */
    } else if ( strcmp(mesg, "CONVERSATION ENDED.\n") == 0 )
	return(DISCONNECT);

    return(mesgptr == '\0' ? nostatus : UNKNOWN);

}   /* End of parsemesg */

/*****************************************************************************/

char *find(str1, str2)

    char	*str1;			/* look for this string */
    char	*str2;			/* in this one */

{

    char	*s1, *s2;		/* can't change str1 or str2 too fast */

/*
 *
 * Looks for *str1 in string *str2. Returns a pointer to the start of the substring
 * if it's found or to the end of string str2 otherwise.
 *
 */ 

    for ( ; *str2 != '\0'; str2++ )  {
	for ( s1 = str1, s2 = str2; *s1 != '\0' && *s1 == *s2; s1++, s2++ ) ;
	if ( *s1 == '\0' )
	    break;
    }	/* End for */

    return(str2);

}   /* End of find */

/*****************************************************************************/

clearline()

{

/*
 *
 * Reads characters from the input line until nothing's left. Don't do anything if
 * we're currently running separate read and write processes.
 * 
 */

    if ( whatami == READWRITE )
	while ( readline() != FALSE ) ;

}   /* End of clearline */

/*****************************************************************************/

sendsignal(sig)

    int		sig;			/* this goes to the other process */

{

/*
 *
 * Sends signal sig to the other process if we're running as separate read and
 * write processes. Returns the result of the kill if there's someone else to
 * signal or -1 if we're running alone.
 *
 */

    if ( whatami != READWRITE && otherpid > 1 )
	return(kill(otherpid, sig));

    return(-1);

}   /* End of sendsignal */

/*****************************************************************************/

void interrupt(sig)

    int		sig;			/* signal that we caught */

{

/*
 *
 * Caught a signal - all except joinsig cause the program to quit. joinsig is the
 * signal sent by the writer to the reader after all the jobs have been transmitted.
 * Used to tell the read process when it can start looking for the end of the job.
 *
 */

    signal(sig, SIG_IGN);

    if ( sig != joinsig )  {
	x_stat |= FATAL;
	if ( canread == TRUE )
	    if ( interactive == FALSE )
		error(NON_FATAL, "signal %d abort", sig);
	    else error(NON_FATAL, "quitting");
	quit(sig);
    }	/* End if */

    writedone = TRUE;
    signal(joinsig, interrupt);

}   /* End of interrupt */

/*****************************************************************************/

logit(mesg, a1, a2, a3)

    char	*mesg;			/* control string */
    unsigned	a1, a2, a3;		/* and possible arguments */

{

/*
 *
 * Simple routine that's used to write a message to the log file.
 *
 */

    if ( mesg != NULL && fp_log != NULL )  {
	fprintf(fp_log, mesg, a1, a2, a3);
	fflush(fp_log);
    }	/* End if */

}   /* End of logit */

/*****************************************************************************/

error(kind, mesg, a1, a2, a3)

    int		kind;			/* FATAL or NON_FATAL error */
    char	*mesg;			/* error message control string */
    unsigned	a1, a2, a3;		/* control string arguments */

{

    FILE	*fp_err;

/*
 *
 * Called when we've run into some kind of program error. First *mesg is printed
 * using the control string arguments a?. If kind is FATAL and we're not ignoring
 * errors the program will be terminated. If mesg is NULL or *mesg is the NULL
 * string nothing will be printed.
 *
 */

    fp_err = (fp_log != NULL) ? fp_log : stderr;

    if ( mesg != NULL && *mesg != '\0' )  {
	fprintf(fp_err, "%s: ", prog_name);
	fprintf(fp_err, mesg, a1, a2, a3);
	putc('\n', fp_err);
    }	/* End if */

    x_stat |= kind;

    if ( kind != NON_FATAL && ignore == OFF )
	quit(SIGTERM);

}   /* End of error */

/*****************************************************************************/

quit(sig)

    int		sig;

{

    int		w;

/*
 *
 * Makes sure everything is properly cleaned up if there's a signal or FATAL error
 * that should cause the program to terminate. The sleep by the write process is
 * to help give the reset sequence a chance to reach the printer before we break
 * the connection - primarily for printers connected to Datakit. There's a very
 * slight chance the reset sequence that's sent to the printer could get us stuck
 * here. Simplest solution is don't bother to send it - everything works without it.
 * Flushing ttyo would be better, but means yet another system dependent procedure
 * in ifdef.c! I'll leave things be for now.
 *
 * Obscure problem on PS-810 turbos says wait a bit after sending an interrupt.
 * Seem to remember the printer getting into a bad state immediately after the
 * top was opened when the toner light was on. A sleep after sending the ctrl-C
 * seemed to fix things.
 *
 */

    signal(sig, SIG_IGN);
    ignore = ON;

    while ( sendsignal(sig) != -1 && (w = wait((int *)0)) != otherpid && w != -1 ) ;

    setupstdin(2);

    if ( currentstate != NOTCONNECTED ) {
	if ( sendctrlC == TRUE ) {
	    Write(ttyo, "\003", 1);
	    Rest(1);			/* PS-810 turbo problem?? */
	}   /* End if */
	Write(ttyo, "\004", 1);
    }	/* End if */

    alarm(0);				/* prevents sleep() loop on V9 systems */
    Rest(2);

    exit(x_stat);

}   /* End of quit */

/*****************************************************************************/

Rest(t)

    int		t;

{

/*
 *
 * Used to replace sleep() calls. Only needed if we're running the program as
 * a read and write process and don't want to have the read process sleep. Most
 * sleeps are in the code because of the non-blocking read used by the single
 * process implementation. Probably should be a macro.
 *
 */

    if ( t > 0 && canwrite == TRUE )
	sleep(t);

}   /* End of Rest */

/*****************************************************************************/

Read(fd, buf, n)

    int		fd;
    char	*buf;
    int		n;

{

    int		count;

/*
 *
 * Used to replace some of the read() calls. Only needed if we're running separate
 * read and write processes. Should only be used to replace read calls on ttyi.
 * Always returns 0 to the caller if the process doesn't have its READ flag set.
 * Probably should be a macro.
 *
 */

    if ( canread == TRUE )  {
	if ( (count = read(fd, buf, n)) == -1 && errno == EINTR )
	    count = 0;
    } else count = 0;

    return(count);

}   /* End of Read */

/*****************************************************************************/

Write(fd, buf, n)

    int		fd;
    char	*buf;
    int		n;

{

    int		count;

/*
 *
 * Used to replace some of the write() calls. Again only needed if we're running
 * separate read and write processes. Should only be used to replace write calls
 * on ttyo. Always returns n to the caller if the process doesn't have its WRITE
 * flag set. Should also probably be a macro.
 *
 */

    if ( canwrite == TRUE )  {
	if ( (count = write(fd, buf, n)) == -1 && errno == EINTR )
	    count = n;
    } else count = n;

    return(count);

}   /* End of Write */
 
/*****************************************************************************/

