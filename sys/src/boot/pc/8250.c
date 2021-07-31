#include "u.h"
#include "lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"

/*
 *  INS8250 uart
 */
enum
{
	/*
	 *  register numbers
	 */
	Data=	0,		/* xmit/rcv buffer */
	Iena=	1,		/* interrupt enable */
	 Ircv=	(1<<0),		/*  for char rcv'd */
	 Ixmt=	(1<<1),		/*  for xmit buffer empty */
	 Irstat=(1<<2),		/*  for change in rcv'er status */
	 Imstat=(1<<3),		/*  for change in modem status */
	Istat=	2,		/* interrupt flag (read) */
	Tctl=	2,		/* test control (write) */
	Format=	3,		/* byte format */
	 Bits8=	(3<<0),		/*  8 bits/byte */
	 Stop2=	(1<<2),		/*  2 stop bits */
	 Pena=	(1<<3),		/*  generate parity */
	 Peven=	(1<<4),		/*  even parity */
	 Pforce=(1<<5),		/*  force parity */
	 Break=	(1<<6),		/*  generate a break */
	 Dra=	(1<<7),		/*  address the divisor */
	Mctl=	4,		/* modem control */
	 Dtr=	(1<<0),		/*  data terminal ready */
	 Rts=	(1<<1),		/*  request to send */
	 Ri=	(1<<2),		/*  ring */
	 Inton=	(1<<3),		/*  turn on interrupts */
	 Loop=	(1<<4),		/*  loop back */
	Lstat=	5,		/* line status */
	 Inready=(1<<0),	/*  receive buffer full */
	 Oerror=(1<<1),		/*  receiver overrun */
	 Perror=(1<<2),		/*  receiver parity error */
	 Ferror=(1<<3),		/*  rcv framing error */
	 Outready=(1<<5),	/*  output buffer empty */
	Mstat=	6,		/* modem status */
	 Ctsc=	(1<<0),		/*  clear to send changed */
	 Dsrc=	(1<<1),		/*  data set ready changed */
	 Rire=	(1<<2),		/*  rising edge of ring indicator */
	 Dcdc=	(1<<3),		/*  data carrier detect changed */
	 Cts=	(1<<4),		/*  complement of clear to send line */
	 Dsr=	(1<<5),		/*  complement of data set ready line */
	 Ring=	(1<<6),		/*  complement of ring indicator line */
	 Dcd=	(1<<7),		/*  complement of data carrier detect line */
	Scratch=7,		/* scratchpad */
	Dlsb=	0,		/* divisor lsb */
	Dmsb=	1,		/* divisor msb */

	Serial=	0,
	Modem=	1,
};

typedef struct Uart	Uart;
struct Uart
{
	int	port;
	uchar	sticky[8];	/* sticky write register values */
	uchar	txbusy;

	void	(*rx)(int);	/* routine to take a received character */
	int	(*tx)(void);	/* routine to get a character to transmit */

	ulong	frame;
	ulong	overrun;
};

static Uart com[2];
static Uart* uart;

#define UartFREQ 1843200

#define uartwrreg(u,r,v)	outb((u)->port + r, (u)->sticky[r] | (v))
#define uartrdreg(u,r)		inb((u)->port + r)

/*
 *  set the baud rate by calculating and setting the baudrate
 *  generator constant.  This will work with fairly non-standard
 *  baud rates.
 */
static void
uartsetbaud(Uart *up, int rate)
{
	ulong brconst;

	brconst = (UartFREQ+8*rate-1)/(16*rate);

	uartwrreg(up, Format, Dra);
	outb(up->port+Dmsb, (brconst>>8) & 0xff);
	outb(up->port+Dlsb, brconst & 0xff);
	uartwrreg(up, Format, 0);
}

/*
 *  toggle DTR
 */
static void
uartdtr(Uart *up, int n)
{
	if(n)
		up->sticky[Mctl] |= Dtr;
	else
		up->sticky[Mctl] &= ~Dtr;
	uartwrreg(up, Mctl, 0);
}

/*
 *  toggle RTS
 */
static void
uartrts(Uart *up, int n)
{
	if(n)
		up->sticky[Mctl] |= Rts;
	else
		up->sticky[Mctl] &= ~Rts;
	uartwrreg(up, Mctl, 0);
}

static void
uartintr(Ureg*, void *arg)
{
	Uart *up;
	int ch;
	int s, l, loops;

	up = arg;
	for(loops = 0; loops < 1024; loops++){
		s = uartrdreg(up, Istat);
		switch(s & 0x3F){
		case 6:	/* receiver line status */
			l = uartrdreg(up, Lstat);
			if(l & Ferror)
				up->frame++;
			if(l & Oerror)
				up->overrun++;
			break;
	
		case 4:	/* received data available */
		case 12:
			ch = inb(up->port+Data);
			if(up->rx)
				(*up->rx)(ch);
			break;
	
		case 2:	/* transmitter empty */
			ch = -1;
			if(up->tx)
				ch = (*up->tx)();
			if(ch != -1)
				outb(up->port+Data, ch);
			else
				up->txbusy = 0;
			break;
	
		case 0:	/* modem status */
			uartrdreg(up, Mstat);
			break;
	
		default:
			if(s&1)
				return;
			print("weird modem interrupt #%2.2ux\n", s);
			break;
		}
	}
	panic("uartintr: 0x%2.2ux\n", uartrdreg(up, Istat));
}

/*
 *  turn on a port's interrupts.  set DTR and RTS
 */
static void
uartenable(Uart *up)
{
	/*
 	 *  turn on interrupts
	 */
	up->sticky[Iena] = 0;
	if(up->tx)
		up->sticky[Iena] |= Ixmt;
	if(up->rx)
		up->sticky[Iena] |= Ircv|Irstat;
	uartwrreg(up, Iena, 0);

	/*
	 *  turn on DTR and RTS
	 */
	uartdtr(up, 1);
	uartrts(up, 1);
}

static void
uartdisable(Uart* up)
{
	/*
 	 * Disable interrupts.
	 */
	up->sticky[Iena] = 0;
	uartwrreg(up, Iena, 0);
	uartdtr(up, 0);
	uartrts(up, 0);
}

void
uartspecial(int port, void (*rx)(int), int (*tx)(void), int baud)
{
	Uart *up;
	int vector;

	switch(port){
	case 0:
		port = 0x3F8;
		vector = VectorUART0;
		up = &com[0];
		break;
	case 1:
		port = 0x2F8;
		vector = VectorUART1;
		up = &com[1];
		break;
	default:
		return;
	}

	if(uart != nil && uart != up)
		uartdisable(uart);
	uart = up;

	if(up->port == 0){
		up->port = port;
		setvec(vector, uartintr, up);
	}

	/*
	 *  set rate to 9600 baud.
	 *  8 bits/character.
	 *  1 stop bit.
	 *  interrupts enabled.
	 */
	uartsetbaud(up, 9600);
	up->sticky[Format] = Bits8;
	uartwrreg(up, Format, 0);
	up->sticky[Mctl] |= Inton;
	uartwrreg(up, Mctl, 0x0);

	up->rx = rx;
	up->tx = tx;
	uartenable(up);
	if(baud)
		uartsetbaud(up, baud);
}

void
uartputc(int c)
{
	int i;
	Uart *up;

	if((up = uart) == nil)
		return;
	for(i = 0; i < 100; i++){
		if(uartrdreg(up, Lstat) & Outready)
			break;
		delay(1);
	}
	outb(up->port+Data, c);
}

void
uartputs(IOQ *q, char *s, int n)
{
	Uart *up;
	int c, x;

	if((up = uart) == nil)
		return;
	while(n--){
		if(*s == '\n')
			q->putc(q, '\r');
		q->putc(q, *s++);
	}
	x = splhi();
	if(up->txbusy == 0 && (c = q->getc(q)) != -1){
		uartputc(c & 0xFF);
		up->txbusy = 1;
	}
	splx(x);
}

void
uartdrain(void)
{
	Uart *up;
	int timeo;

	if((up = uart) == nil)
		return;
	for(timeo = 0; timeo < 10000 && up->txbusy; timeo++)
		delay(1);
}
