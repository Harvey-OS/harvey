#include <u.h>
#include <libc.h>
#include "dat.h"

Place nowhere = {
	Undef, Undef
};

static void
setlat(Place *p, double lat)
{
	p->lat = lat;
}

static void
setlon(Place *p, double lon)
{
	p->lon = lon;
}

static int
printlatlon(char *p, int n, double lat, char po, char ne)
{
	char c;
	double d;
	int deg, min, sec;

	if(lat > 0)
		c = po;
	else if(lat < 0){
		c = ne;
		lat = -lat;
	} else
		c = ' ';
	sec = 3600 * modf(lat, &d);
	deg = d;
	min = sec/60;
	sec = sec % 60;
	return snprint(p, n, "%#3.3d°%#2.2d′%#2.2d″%c", deg, min, sec, c);
}

int
placeconv(Fmt *fp)
{
	char str[256];
	int n;
	Place pl;

	pl = va_arg(fp->args, Place);
	n = 0;
	n += printlatlon(str+n, sizeof(str)-n, pl.lat, 'N', 'S');
	n += snprint(str+n, sizeof(str)-n, ", ");
	printlatlon(str+n, sizeof(str)-n, pl.lon, 'E', 'W');
	return fmtstrcpy(fp, str);
}

int
strtolatlon(char *p, char **ep, Place *pl)
{
	double latlon;
	int neg = 0;

	while(*p == '0') p++;
	latlon = strtod(p, &p);
	if(latlon < 0){
		latlon = -latlon;
		neg = 1;
	}
	if(*p == ':'){
		p++;
		while(*p == '0') p++;
		latlon += strtod(p, &p)/60.0;
		if(*p == ':'){
			p++;
			while(*p == '0') p++;
			latlon += strtod(p, &p)/3600.0;
		}
	}
	switch (*p++){
	case 'N':
	case 'n':
		if(neg) latlon = -latlon;
		if(pl->lat != Undef)
			return -1;
		setlat(pl, latlon);
		break;
	case 'S':
	case 's':
		if(!neg) latlon = -latlon;
		if(pl->lat != Undef)
			return -1;
		setlat(pl, latlon);
		break;
	case 'E':
	case 'e':
		if(neg) latlon = -latlon;
		if(pl->lon != Undef)
			return -1;
		setlon(pl, latlon);
		break;
	case 'W':
	case 'w':
		if(!neg) latlon = -latlon;
		if(pl->lon != Undef)
			return -1;
		setlon(pl, latlon);
		break;
	case '\0':
	case ' ':
	case '\t':
	case '\n':
		p--;
		if(neg) latlon = -latlon;
		if(pl->lat == Undef){
			setlat(pl, latlon);
		} else if(pl->lon == Undef){
			latlon = -latlon;
			setlon(pl, latlon);
		} else return -1;
		break;
	default:
		return -1;
	}
	if(ep)
		*ep = p;
	return 0;
}

Place
strtopos(char *p, char **ep)
{
	Place pl = nowhere;

	if(strtolatlon(p, &p, &pl) < 0)
		return nowhere;
	while(*p == ' ' || *p == '\t' || *p == '\n')
		p++;
	if(strtolatlon(p, &p, &pl) < 0)
		return nowhere;
	if(ep)
		*ep = p;
	return pl;
}

static void
rtcset(long t)		/* We may use this some day */
{
	static int fd;
	long r;
	int n;
	char buf[32];

	if(fd <= 0 && (fd = open("#r/rtc", ORDWR)) < 0){
		fprint(2, "Can't open #r/rtc: %r\n");
		return;
	}
	n = read(fd, buf, sizeof buf - 1);
	if(n <= 0){
		fprint(2, "Can't read #r/rtc: %r\n");
		return;
	}
	buf[n] = '\0';
	r = strtol(buf, nil, 0);
	if(r <= 0){
		fprint(2, "ridiculous #r/rtc: %ld\n", r);
		return;
	}
	if(r - t > 1 || t - r > 0){
		seek(fd, 0, 0);
		fprint(fd, "%ld", t);
		fprint(2, "correcting #r/rtc: %ld → %ld\n", r, t);
	}
	seek(fd, 0, 0);
}
