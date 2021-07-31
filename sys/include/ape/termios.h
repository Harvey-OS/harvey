#pragma lib "/$M/lib/ape/libap.a"
/* input modes */
#define BRKINT	0x001
#define ICRNL	0x002
#define IGNBRK	0x004
#define IGNCR	0x008
#define IGNPAR	0x010
#define INLCR	0x020
#define INPCK	0x040
#define ISTRIP	0x080
#define IXOFF	0x100
#define IXON	0x200
#define PARMRK	0x400

/* output modes: ONLCR is an extension to POSIX! */
#define OPOST	0x001
#define ONLCR	0x002

/* control modes */
#define CLOCAL	0x001
#define CREAD	0x002
#define CSIZE	0x01C
#define CS5	0x004
#define CS6	0x008
#define CS7	0x00C
#define CS8	0x010
#define CSTOPB	0x020
#define HUPCL	0x040
#define PARENB	0x080
#define PARODD	0x100

/* local modes */
#define ECHO	0x001
#define ECHOE	0x002
#define ECHOK	0x004
#define ECHONL	0x008
#define ICANON	0x010
#define IEXTEN	0x020
#define ISIG	0x040
#define NOFLSH	0x080
#define TOSTOP	0x100

/* control characters */
#define VEOF	0
#define VEOL	1
#define VERASE	2
#define VINTR	3
#define VKILL	4
#define VMIN	5
#define VQUIT	6
#define VSUSP	7
#define VTIME	8
#define VSTART	9
#define VSTOP	10
#define NCCS	11

/* baud rates */
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
#define B1800	10
#define B2400	11
#define B4800	12
#define B9600	13
#define B19200	14
#define B38400	15

/* optional actions for tcsetattr */
#define TCSANOW	  1
#define TCSADRAIN 2
#define TCSAFLUSH 3

typedef unsigned long tcflag_t;
typedef unsigned long speed_t;
typedef unsigned char cc_t;

struct termios {
	tcflag_t	c_iflag;	/* input modes */
	tcflag_t	c_oflag;	/* output modes */
	tcflag_t	c_cflag;	/* control modes */
	tcflag_t	c_lflag;	/* local modes */
	cc_t		c_cc[NCCS];	/* control characters */
};

extern speed_t cfgetospeed(const struct termios *);
extern int cfsetospeed(struct termios *, speed_t);
extern speed_t cfgetispeed(const struct termios *);
extern int cfsetispeed(struct termios *, speed_t);
extern int tcgetattr(int, struct termios *);
extern int tcsetattr(int, int, const struct termios *);
extern int tcsendbreak(int, int);
extern int tcdrain(int);
extern int tcflush(int, int);
extern int tcflow(int, int);
