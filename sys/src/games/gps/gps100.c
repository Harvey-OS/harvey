#include	<u.h>
#include	<libc.h>
#include	<bio.h>


typedef	struct	Shared	Shared;

/*
 * z 0 '0 '0 '6 '7 '3
 * A 6 '0 '4 '0 '  '4 '1 '. '0 '6 '' 'N
 * B 7 '0 '7 '4 '  '2 '3 '. '9 '3 '' 'W
 * C 9 '0 '0 '0
 * D 5 '0 '0 '0
 * E 3 '0 '0 '0 '0 '. '0
 * G c 'L '0 '0 '. '0 '0
 * I 4 '0 '0 '0 '. '0
 * K a '_ '_ '_ '_ '_
 * L 2 '0 '0 '0 '. '0
 * Q 8 'W '1 '2 '. '9
 *	missing fields
 *		S nav flag
 *		T warning status
 */
struct	Shared
{
	char	fieldz[6];	/* current altitude 00673 */
	char	fieldA[12];	/* current latitude 040 41.06'N */
	char	fieldB[12];	/* current longitude 074 23.93'W */
	char	fieldC[6];	/* track in whole degrees */
	char	fieldD[6];	/* ground speed in knots */
	char	fieldE[8];	/* distance to wpt in tenths nmile */
	char	fieldG[8];	/* cross track error */
	char	fieldI[6];	/* desired track in tenths deg */
	char	fieldK[6];	/* destination wpt id */
	char	fieldL[6];	/* bearing to destination wpt tenths deg */
	char	fieldQ[6];	/* magnetic variation */
	long	lastime;
	long	lastprint;
} data;

int	state;
enum
{
	Lookaa1		= 1,
	Lookac,
	Lookaa2,

	Aa		= 0xaa,
	Ac		= 0xac,
};

void	dumpdatabase(void);
void	storerec(char*, int);
void	dofile(int);

char	eiafile[] = "/dev/eia0";
char	gpslog[] = "/tmp/gpslog";
uchar	record[1000];
uchar	badmsg[256];
int	recstate;
int	recp;
int	deltaalt;

void
main(int argc, char *argv[])
{
	int c, fi, fl;
	Biobuf gpb;
	char *p;

	ARGBEGIN {
	case 'A':
		deltaalt = atol(ARGF());
		break;
	} ARGEND
	p = eiafile;
	if(argc > 0)
		p = argv[0];
	fi = open(p, OREAD);
	if(fi < 0) {
		fprint(2, "cannot open %s\n", p);
		goto out;
	}
	Binit(&gpb, fi, OREAD);

	fl = open(gpslog, OWRITE);
	seek(fl, 0, 2);
	if(fl < 0)
		fprint(2, "no gps log %s\n", gpslog);

	recstate = Lookaa1;
	memset(&data, 0, sizeof(data));
	memset(badmsg, 0, sizeof(badmsg));
	data.lastprint = time(0);
	recp = 0;

loop:
	c = Bgetc(&gpb);
	if(c < 0)
		goto out;
	switch(recstate) {
	default:
	case Lookaa1:
		if(c == Aa)
			recstate = Lookac;
		else
			recstate = Lookaa1;
		goto common;

	case Lookac:
		if(c == Ac)
			recstate = Lookaa2;
		else
		if(c == Aa)
			recstate = Lookac;
		else
			recstate = Lookaa1;
		goto common;

	case Lookaa2:
		if(c == Aa)
			break;
		recstate = Lookaa1;
		goto common;

	common:
		if(recp >= sizeof(record))
			recp = 0;
		record[recp++] = c;
		goto loop;
	}
	recp -= 2;
	if(recp >= 2) {
		record[recp] = 0;
		c = record[0];
		switch(c) {
		default:
			if(badmsg[c] == 0) {
				print("bad message %.2x %.2x %s\n", c, record[1], record+2);
				badmsg[c] = 1;
			}
			break;
		case 0x00:
			storerec(data.fieldz, sizeof(data.fieldz));
			break;	/**/
	/*	case 0x02:
			storerec(data.fieldL, sizeof(data.fieldL));
			break;	/**/
	/*	case 0x03:
			storerec(data.fieldE, sizeof(data.fieldE));
			break;	/**/
	/*	case 0x04:
			storerec(data.fieldI, sizeof(data.fieldI));
			break;	/**/
		case 0x05:
			storerec(data.fieldD, sizeof(data.fieldD));
			break;	/**/
		case 0x06:
			storerec(data.fieldA, sizeof(data.fieldA));
			break;	/**/
		case 0x07:
			storerec(data.fieldB, sizeof(data.fieldB));
			break;	/**/
		case 0x08:
			storerec(data.fieldQ, sizeof(data.fieldQ));
			dofile(fl);
			break;	/**/
		case 0x09:
			storerec(data.fieldC, sizeof(data.fieldC));
			break;	/**/
	/*	case 0x0a:
			storerec(data.fieldK, sizeof(data.fieldK));
			break;	/**/
	/*	case 0x0c:
			storerec(data.fieldG, sizeof(data.fieldG));
			break;	/**/
		}
	}
	recp = 0;
	goto loop;

out:
	exits("error");
}

void
storerec(char *p, int len)
{
	int l;

	l = strlen((char*)record+2);
	if(l >= len)
		l = len-1;
	memset(p, 0, len);
	memcpy(p, record+2, l);
}

void
strip(char *p, int n)
{
	int i;

	for(i=0; i<n && p[i]=='0'; i++)
		p[i] = ' ';
}

void
dofile(int fl)
{
	char databuf[100];
	int alt, len;

	if(data.fieldA[0] == 0)
		return;
	if(data.fieldB[0] == 0)
		return;
	if(data.fieldz[0] == 0)
		return;
	if(data.fieldC[0] == 0)
		return;
	if(data.fieldD[0] == 0)
		return;
	if(data.fieldQ[0] == 0)
		return;

	data.lastime = time(0);

	strip(data.fieldA, 2);
	strip(data.fieldB, 2);
	strip(data.fieldz, 4);
	strip(data.fieldC, 2);
	strip(data.fieldD, 2);
	strip(data.fieldQ, 3);

	alt = atol(data.fieldz) + deltaalt;

	sprint(databuf, "%11ld %.3s%c%s %.3s%c%s %5d %s %s %s\n",
		data.lastime,
		data.fieldA, L'°', data.fieldA+4,
		data.fieldB, L'°', data.fieldB+4,
		alt, data.fieldC, data.fieldD, data.fieldQ);

	len = strlen(databuf);
	if(len != 58)
		print("len = %d\n", len);

	if(fl >= 0)
		write(fl, databuf, len);
	if(data.lastime > data.lastprint) {
		write(1, databuf, len);
		data.lastprint = time(0) + 30;
	}
}
