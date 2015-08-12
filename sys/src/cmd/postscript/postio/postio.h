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
 * POSTBEGIN, if it's not NULL, is some PostScript code that's sent to the
 *printer
 * before any of the input files. It's not terribly important since the same
 *thing
 * can be accomplished in other ways, but this approach is convenient. POSTBEGIN
 * is initialized so as to disable job timeouts. The string can also be set on
 *the
 * command line using the -P option.
 *
 */

#define POSTBEGIN "statusdict /waittimeout 0 put\n"

/*
 *
 * The following help determine where postio is when it's running - either in
 *the
 * START, SEND, or DONE states. Primarily controls what's done in getstatus().
 * RADIAN occasionally had problems with two way conversations. Anyway this
 *stuff
 * can be used to prevent status queries while we're transmitting a job. Enabled
 * by the -q option.
 *
 */

#define NOTCONNECTED 0
#define START 1
#define SEND 2
#define DONE 3

/*
 *
 * Previous versions of postio only ran as a single process. That was (and still
 * is) convenient, but meant we could only flow control one direction. Data
 *coming
 * back from the printer occasionally got lost, but that didn't often hurt
 *(except
 * for lost error messages). Anyway I've added code that lets you split the
 *program
 * into separate read and write processes, thereby helping to prevent data loss
 *in
 * both directions. It should be particularly useful when you're sending a job
 *that
 * you expect will be returning useful data over the communications line.
 *
 * The next three definitions control what's done with data on communications
 *line.
 * The READ flag means the line can be read, while the WRITE flag means it can
 *be
 * written. When we're running as a single process both flags are set. I tried
 *to
 * overlay the separate read/write process code on what was there and working
 *for
 * one process. The implementation isn't as good as it could be, but should be
 * safe. The single process version still works, and remains the default.
 *
 */

#define READ 1
#define WRITE 2
#define READWRITE 3

/*
 *
 * Messages generated on the printer and returned over the communications line
 * look like,
 *
 *	%%[ status: idle; source: serial 25 ]%%
 *	%%[ status: waiting; source: serial 25 ]%%
 *	%%[ status: initializing; source: serial 25 ]%%
 *	%%[ status: busy; source: serial 25 ]%%
 *	%%[ status: printing; source: serial 25 ]%%
 *	%%[ status: PrinterError: out of paper; source: serial 25 ]%%
 *	%%[ status: PrinterError: no paper tray; source: serial 25 ]%%
 *
 *	%%[ PrinterError: out of paper; source: serial 25 ]%%
 *	%%[ PrinterError: no paper tray; source: serial 25 ]%%
 *
 *	%%[ Error: undefined; OffendingCommand: xxx ]%%
 *	%%[ Flushing: rest of job (to end-of-file) will be ignored ]%%
 *
 * although the list isn't meant to be complete.
 *
 * The following constants are used to classify the recognized printer states.
 * readline() reads complete lines from ttyi and stores them in array mesg[].
 * getstatus() looks for the "%%[ " and " ]%%" delimiters that bracket printer
 * messages and if found it tries to parse the enclosed message. After the
 *lookup
 * one of the following numbers is returned as an indication of the existence or
 * content of the printer message. The return value is used in start(), send(),
 * and done() to figure out what's happening and what can be done next.
 *
 */

#define BUSY 0         /* processing data already sent */
#define WAITING 1      /* printer wants more data */
#define PRINTING 2     /* printing a page */
#define IDLE 3         /* ready to start the next job */
#define ENDOFJOB 4     /* readline() builds this up on EOF */
#define PRINTERERROR 5 /* PrinterError - eg. out of paper */
#define ERROR 6        /* some kind of PostScript error */
#define FLUSHING 7     /* throwing out the rest of the job */
#define INITIALIZING 8 /* printer is booting */
#define DISCONNECT 9   /* from Datakit! */
#define UNKNOWN 10     /* in case we missed anything */
#define NOSTATUS 11    /* no response from the printer */

#define WRITEPROCESS 12 /* dummy states for write process */
#define INTERACTIVE 13  /* and interactive mode */

/*
 *
 * An array of type Status is used, in getstatus(), to figure out the printer's
 * current state. Just helps convert strings representing the current state into
 * integer codes that other routines use.
 *
 */

typedef struct {
	char* state; /* printer's current status */
	int val;     /* value returned by getstatus() */
} Status;

/*
 *
 * STATUS is used to initialize an array of type Status that translates the
 *ASCII
 * strings returned by the printer into appropriate codes that can be used later
 * on in the program. getstatus() converts characters to lower case, so if you
 * add any entries make them lower case and put them in before the UNKNOWN
 *entry.
 * The lookup terminates when we get a match or when an entry with a NULL state
 * is found.
 *
 */

#define STATUS                                                                 \
                                                                               \
	{                                                                      \
		"busy", BUSY, "waiting", WAITING, "printing", PRINTING,        \
		    "idle", IDLE, "endofjob", ENDOFJOB, "printererror",        \
		    PRINTERERROR, "error", ERROR, "flushing", FLUSHING,        \
		    "initializing", INITIALIZING, NULL, UNKNOWN                \
	}

/*
 *
 * The baud rate can be set on the command line using the -b option. If you omit
 * it BAUDRATE will be used.
 *
 */

#define BAUDRATE B9600

/*
 *
 * An array of type Baud is used, in routine getbaud(), to translate ASCII
 *strings
 * into termio values that represent the requested baud rate.
 *
 */

typedef struct {
	char* rate; /* string identifying the baud rate */
	short val;  /* and its termio.h value */
} Baud;

/*
 *
 * BAUDTABLE initializes the array that's used to translate baud rate requests
 * into termio values. It needs to end with an entry that has NULL assigned to
 * the rate field.
 *
 */

#define BAUDTABLE                                                              \
                                                                               \
	{                                                                      \
		"9600", B9600, "B9600", B9600, "19200", EXTA, "19.2", EXTA,    \
		    "B19200", EXTA, "EXTA", EXTA, "1200", B1200, "B1200",      \
		    B1200, "2400", B2400, "B2400", B2400, "B4800", B4800,      \
		    "4800", B4800, "38400", EXTB, "38.4", EXTB, "B38400",      \
		    EXTB, "EXTB", EXTB, NULL, B9600                            \
	}

/*
 *
 * A few miscellaneous definitions. BLOCKSIZE is the default size of the buffer
 * used for reading the input files (changed with the -B option). MESGSIZE is
 *the
 * size of the character array used to store printer status lines - don't make
 *it
 * too small!
 *
 */

#define BLOCKSIZE 2048
#define MESGSIZE 512

/*
 *
 * Some of the non-integer valued functions used in postio.c.
 *
 */

char* find();

char* malloc();
char* strtok();
