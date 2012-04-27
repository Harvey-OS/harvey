#include <u.h>
#include <libc.h>
#include "dat.h"

char *serial = "/dev/eia0";

int ttyfd, ctlfd, debug;
int baud = Baud;
char *baudstr = "b%dd1r1pns1l8i1w5";

Place where = {-(74.0 + 23.9191/60.0), 40.0 + 41.1346/60.0};

void	setline(void);
void	evermore80(Place, int);
void	evermore89(int);
void	evermore8e(void);

void
setline(void){
	char *serialctl;

	serialctl = smprint("%sctl", serial);
	if((ttyfd = open(serial, ORDWR)) < 0)
		sysfatal("%s: %r", serial);
	if((ctlfd = open(serialctl, OWRITE)) >= 0){
		if(fprint(ctlfd, baudstr, baud) < 0)
			sysfatal("%s: %r", serialctl);
	}
	free(serialctl);
}

enum {
	GGAon = 0x01,
	GLLon = 0x02,
	GSAon = 0x04,
	GSVon = 0x08,
	RMCon = 0x10,
	VTGon = 0x20,
	CRCon = 0x40,
	EMTon = 0x80
};

char*
putbyte(char *s, int v)
{
	*s++ = v;
	if((v & 0xff) == 0x10)
		*s++ = v;
	return s;
}

char*
putshort(char *s, int v)
{
	s = putbyte(s, v);
	s = putbyte(s, v >> 8);
	return s;
}

char*
putlong(char *s, long v)
{
	s = putbyte(s, v);
	s = putbyte(s, v >> 8);
	s = putbyte(s, v >> 16);
	s = putbyte(s, v >> 24);
	return s;
}

void
evermoresend(char *body, int l)
{
	char buf[8], *s;
	int crc, i;

	s = buf;
	*s++ = 0x10;			/* DCE */
	*s++ = 0x02;			/* STX */
	s = putbyte(s, l);		/* length */
	write(ttyfd, buf, s-buf);	/* write header */

	write(ttyfd, body, l);		/* write body */

	crc = 0;
	for(i = 0; i < l; i++)
		crc += body[i];		/* calculate crc */
	s = buf;
	s = putbyte(s, crc);		/* checksum */
	*s++ = 0x10;			/* DCE */
	*s++ = 0x03;			/* ETX */
	write(ttyfd, buf, s-buf);	/* write trailer */
}

void
evermore80(Place pl, int baud)
{
	char buf[32], *s;
	long now, seconds, week;

	fprint(2, "Evermore80");

	time(&now);
	seconds = now - 315964800;
	week = (seconds / (7*24*3600));
	seconds = seconds %  (7*24*3600);
	s = buf;

	s = putbyte(s, 0x80);		/* message ID */
	s = putshort(s, week);		/* week number */
	s = putlong(s, seconds*100);	/* seconds */
	s = putshort(s, pl.lat*10.0);	/* latitude tenths degree */
	s = putshort(s, pl.lon*10.0);	/* longitude tenths degree */
	s = putshort(s, 100);		/* altitude meters */
	s = putshort(s, 0);		/* datumn ID */
	s = putbyte(s, 2);		/* warm start */
	s = putbyte(s, GGAon|GSAon|GSVon|RMCon|CRCon);
	switch(baud){
	case 4800:	s = putbyte(s, 0);	break;
	case 9600:	s = putbyte(s, 1);	break;
	case 19200:	s = putbyte(s, 2);	break;
	case 38400:	s = putbyte(s, 3);	break;
	default:
		sysfatal("Illegal baud rate");
	}

	evermoresend(buf, s - buf);
	fprint(2, "\n");
}

void
evermore89(int baud)
{
	char buf[32], *s;

	fprint(2, "Evermore89");
	s = buf;
	s = putbyte(s, 0x89);		/* message ID */
	s = putbyte(s, 0x01);		/* set main serial port */
	switch(baud){
	case  4800:	s = putbyte(s, 0x00);	break;
	case  9600:	s = putbyte(s, 0x01);	break;
	case 19200:	s = putbyte(s, 0x02);	break;
	case 38400:	s = putbyte(s, 0x03);	break;
	default:
		sysfatal("illegal baud rate %d", baud);
	}

	evermoresend(buf, s - buf);
	fprint(2, "\n");
}

void
evermore8e(void)
{
	char buf[32], *s;

	fprint(2, "Evermore8e");
	s = buf;
	s = putbyte(s, 0x8e);		/* message ID */
	s = putbyte(s, GGAon|GSAon|GSVon|RMCon);		/* all messages except GLL and VTG */
	s = putbyte(s, 0x01);		/* checksum on */
	s = putbyte(s, 0x01);		/* GGA update rate */
	s = putbyte(s, 0x0b);		/* GLL update rate */
	s = putbyte(s, 0x0a);		/* GSA update rate */
	s = putbyte(s, 0x14);		/* GSV update rate */
	s = putbyte(s, 0x08);		/* RMC update rate */
	s = putbyte(s, 0x0d);		/* VTG update rate */

	evermoresend(buf, s - buf);
	fprint(2, "\n");
}

void
main(int argc, char*argv[])
{
	char *p;
	Place pl;
	int newbaud;

	newbaud = -1;
	pl = nowhere;
	ARGBEGIN {
	default:
		fprint(2, "usage: %s [-b baud] [-d device] [-l longitude latitude] [-n newbaud]\n", argv0);
		exits("usage");
	case 'D':
		debug++;
		break;
	case 'b':
		baud = strtol(ARGF(), nil, 0);
		break;
	case 'd':
		serial = ARGF();
		break;
	case 'l':
		p = ARGF();
		if(strtolatlon(p, &p, &pl) < 0)
			sysfatal("bad position");
		while(*p == ' ' || *p == '\t' || *p == '\n')
			p++;
		if(*p == '\0')
			p = ARGF();
		if (strtolatlon(p, &p, &pl) < 0)
			sysfatal("bad position");
		while(*p == ' ' || *p == '\t' || *p == '\n')
			p++;
		if(*p != '\0')
			sysfatal("trailing gunk in position");
		where = pl;
		break;
	case 'n':
		newbaud = strtol(ARGF(), nil, 0);
		break;
	} ARGEND

	if(newbaud < 0)
		newbaud = baud;

	fmtinstall('L', placeconv);
	print("Initializing GPS to %d baud, at %L, time %s\n",
		newbaud, where, ctime(time(nil)));
	setline();
	evermore80(where, newbaud);
}
