/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

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
