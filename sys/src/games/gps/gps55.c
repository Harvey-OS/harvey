#include	<u.h>
#include	<libc.h>
#include	<bio.h>

char	eiafile[] = "/dev/eia0";
char	gpslog[] = "/tmp/gpslog";
char	record[150];
char	databuf[150];
int	deltaalt;

long	gethhmmss(char*);
long	getddmmyy(char*);
long	getnumb(char*);

void
main(int argc, char *argv[])
{
	int c, fi, fl;
	Biobuf gpb;
	char *p;
	long t0, t1, alt, lat, lng, speed, magvar, dir;
	int latns, lngew, magew;

	ARGBEGIN {
	case 'A':
		deltaalt = atol(ARGF());
		break;
	} ARGEND
	p = eiafile;
	if(argc > 0)
		p = argv[0];
	sprint(record, "%sctl", p);
	fi = open(record, OWRITE);
	if(fi >= 0) {
		write(fi, "b4800", 5);
		close(fi);
	}
	fi = open(p, OREAD);
	if(fi < 0) {
		fprint(2, "cannot open %s\n", p);
		goto out;
	}
	Binit(&gpb, fi, OREAD);

	fl = open(gpslog, OWRITE);
	if(fl < 0) {
		fl = create(gpslog, OWRITE, 0666);
		if(fl < 0) {
			fprint(2, "cant create %s\n", gpslog);
			exits("create");
		}
	}
	seek(fl, 0, 2);

loop:
	p = record;
	for(;;) {
		c = Bgetc(&gpb);
		if(c < 0)
			goto out;
		c &= 0x7f;
		if(p >= record+sizeof(record))
			p = record;
		*p++ = c;
		if(c == '\n')
			break;
	}
	if(memcmp(record, "$GPRMC", 6))
		goto loop;
	p[-2] = 0;

	/* hhmmss 232310 */
	p = strchr(record, ',');
	if(p == 0)
		goto loop;
	p++;
	t1 = gethhmmss(p);
	if(t1 < 0)
		goto loop;

	/* alt A */
	p = strchr(p, ',');
	if(p == 0)
		goto loop;
	p++;
	alt = getnumb(p);
	if(alt < 0)
		alt = 0;
	else
		alt = alt/100 + deltaalt;

	/* lat 4042.11 */
	p = strchr(p, ',');
	if(p == 0)
		goto loop;
	p++;
	lat = getnumb(p);
	if(lat < 0)
		goto loop;

	/* NS N */
	p = strchr(p, ',');
	if(p == 0)
		goto loop;
	p++;
	latns = *p;
	if(latns != 'N' && latns != 'S')
		goto loop;

	/* lng 07424.28 */
	p = strchr(p, ',');
	if(p == 0)
		goto loop;
	p++;
	lng = getnumb(p);
	if(lng < 0)
		goto loop;

	/* EW W */
	p = strchr(p, ',');
	if(p == 0)
		goto loop;
	p++;
	lngew = *p;
	if(lngew != 'E' && lngew != 'W')
		goto loop;

	/* speed 032.2 */
	p = strchr(p, ',');
	if(p == 0)
		goto loop;
	p++;
	speed = getnumb(p);

	/* dir 315.6 */
	p = strchr(p, ',');
	if(p == 0)
		goto loop;
	p++;
	dir = getnumb(p);

	/* ddmmyy 081292 */
	p = strchr(p, ',');
	if(p == 0)
		goto loop;
	p++;
	t0 = getddmmyy(p);
	if(t0 < 0)
		goto loop;

	/* magvar 012.9 */
	p = strchr(p, ',');
	if(p == 0)
		goto loop;
	p++;
	magvar = getnumb(p);

	/* EW W*72 */
	p = strchr(p, ',');
	if(p == 0)
		goto loop;
	p++;
	magew = *p;
	if(magew != 'E' && magew != 'W')
		goto loop;

	sprint(databuf, "%11ld %3d°%.2d.%.2d'%c %3d°%.2d.%.2d'%c %5d %3d %3d %c%2d.%d\n",
		t0+t1,
		lat/10000, lat%10000/100, lat%100, latns,
		lng/10000, lng%10000/100, lng%100, lngew,
		alt/100, speed/100, dir/100,
		magew, magvar/100, magvar%100/10);

	c = strlen(databuf);
	if(c != 58)
		print("len = %d\n", c);
	if(fl >= 0)
		write(fl, databuf, c);
	goto loop;

out:
	exits("error");
}

long
getddmmyy(char *p)
{
	static lastd, lastm, lasty, lastt;
	int d, m, y, c;
	long t;
	Tm *tm;

	c = p[0] - '0';
	if(c < 0 || c > 9)
		goto bad;
	d = c*10;
	c = p[1] - '0';
	if(c < 0 || c > 9)
		goto bad;
	d += c;

	c = p[2] - '0';
	if(c < 0 || c > 9)
		goto bad;
	m = c*10;
	c = p[3] - '0';
	if(c < 0 || c > 9)
		goto bad;
	m += c;
	m--;

	c = p[4] - '0';
	if(c < 0 || c > 9)
		goto bad;
	y = c*10;
	c = p[5] - '0';
	if(c < 0 || c > 9)
		goto bad;
	y += c;
	if(d == lastd && m == lastm && y == lasty)
		return lastt;

	t = 0;
	for(c=1000000; c; t+=c) {
		tm = gmtime(t);
		if(tm->year < y)
			continue;
		if(tm->year > y)
			goto dec;
		if(tm->mon < m)
			continue;
		if(tm->mon > m)
			goto dec;
		if(tm->mday < d)
			continue;
		if(tm->mday > d)
			goto dec;
		if(tm->hour || tm->min || tm->sec)
			goto dec;
		break;
	dec:
		t -= c;
		c /= 10;
	}
	lastd = d;
	lastm = m;
	lasty = y;
	lastt = t;
	return t;

bad:
	return -1;
}

long
gethhmmss(char *p)
{
	int c;
	long t;

	c = p[0] - '0';
	if(c < 0 || c > 9)
		goto bad;
	t = c*36000;
	c = p[1] - '0';
	if(c < 0 || c > 9)
		goto bad;
	t += c*3600;
	c = p[2] - '0';
	if(c < 0 || c > 9)
		goto bad;
	t += c*600;
	c = p[3] - '0';
	if(c < 0 || c > 9)
		goto bad;
	t += c*60;
	c = p[4] - '0';
	if(c < 0 || c > 9)
		goto bad;
	t += c*10;
	c = p[5] - '0';
	if(c < 0 || c > 9)
		goto bad;
	t += c*1;
	return t;

bad:
	return -1;
}

long
getnumb(char *p)
{
	int c;
	long t;

	for(t=0;;) {
		c = *p++;
		if(c == '.' || c == ',')
			break;
		if(c < '0' || c > '9')
			goto bad;
		t = t*10 + (c-'0');
	}
	t = t*100;
	if(c == '.') {
		c = *p++;
		if(c != ',') {
			if(c < '0' || c > '9')
				goto bad;
			t = t + (c-'0')*10;

			c = *p;
			if(c != ',') {
				if(c < '0' || c > '9')
					goto bad;
				t = t + (c-'0');
			}
		}
	}
	return t;

bad:
	return -1;
}
