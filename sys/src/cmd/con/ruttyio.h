/*
 * teletype-related ioctls
 */

struct sgttyb {
	char	sg_ispeed;		/* no longer set/returned by kernel */
	char	sg_ospeed;		/* no longer set/returned by kernel */
	char	sg_erase;		/* erase character */
	char	sg_kill;		/* kill character */
	unsigned char	sg_flags[2];	/* mode flags, low byte first */
};

/*
 * Structure for setting and getting tty device parammeters.
 */
struct ttydevb {
	char	ispeed;			/* input speed */
	char	ospeed;			/* output speed */
	unsigned char	flags[2];		/* mode flags, low byte first */
};

/*
 * List of special characters
 */
struct tchars {
	char	t_intrc;	/* interrupt */
	char	t_quitc;	/* quit */
	char	t_startc;	/* start output */
	char	t_stopc;	/* stop output */
	char	t_eofc;		/* end-of-file */
	char	t_brkc;		/* input delimiter (like nl) */
};

/*
 * modes in sg_flags
 */
#define	TANDEM	01
#define	CBREAK	02
#define	LCASE	04
#define	ECHO	010
#define	CRMOD	020
#define	RAW	040
/* 0300: former parity bits */
#define	NLDELAY	001400
#define	TBDELAY	006000
#define	XTABS	06000
#define	CRDELAY	030000
#define	VTDELAY	040000
#define BSDELAY 0100000
#define ALLDELAY 0177400

/*
 * Delay algorithms
 */
#define	CR0	0
#define	CR1	010000
#define	CR2	020000
#define	CR3	030000
#define	NL0	0
#define	NL1	000400
#define	NL2	001000
#define	NL3	001400
#define	TAB0	0
#define	TAB1	002000
#define	TAB2	004000
#define	FF0	0
#define	FF1	040000
#define	BS0	0
#define	BS1	0100000

/*
 * Speeds
 */
#define B0	0
#define B50	1
#define B75	2
#define B110	3
#define B134	4
#define B150	5
#define B200	6
#define B300	7
#define B600	8
#define B1200	9
#define	B1800	10
#define B2400	11
#define B4800	12
#define B9600	13
#define EXTA	14
#define EXTB	15

/*
 * device flags
 */

#define	F8BIT	040	/* eight-bit path */
#define	ODDP	0100
#define	EVENP	0200
#define ANYP	0300

/*
 * tty ioctl commands
 */
#define	TIOCHPCL	(('t'<<8)|2)
#define	TIOCGETP	(('t'<<8)|8)
#define	TIOCSETP	(('t'<<8)|9)
#define	TIOCSETN	(('t'<<8)|10)
#define	TIOCEXCL	(('t'<<8)|13)
#define	TIOCNXCL	(('t'<<8)|14)
#define	TIOCFLUSH	(('t'<<8)|16)
#define	TIOCSETC	(('t'<<8)|17)
#define	TIOCGETC	(('t'<<8)|18)
#define	TIOCSBRK	(('t'<<8)|19)
#define TIOCGDEV	(('t'<<8)|23)	/* get device parameters */
#define TIOCSDEV	(('t'<<8)|24)	/* set device parameters */
#define	TIOCSPGRP	(('t'<<8)|118)	/* set pgrp of tty */
#define	TIOCGPGRP	(('t'<<8)|119)	/* get pgrp of tty */
