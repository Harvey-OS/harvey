#include	<all.h>


/*	  691132205  40°°47.96'N  74°°25.48'W    76   0   0 W12.9n
 *	0000000000111111111122222222223333333333444444444455555555
 *	0123456789012345678901234567890123456789012345678901234567
 */
int
getgps(Loc *loc)
{
	char buf[Gpsrsize];
	int n, deg, min, cmin;
	double lat, lng, xx;
	static double olat, olng;

	if(map.gpsfd < 0) {
		map.gpsfd = open(map.glog, OREAD);
		if(map.gpsfd < 0)
			goto bad;
	}

loop:
	seek(map.gpsfd, -Gpsrsize, 2);
	n = read(map.gpsfd, buf, Gpsrsize);
	if(n < 0) {
		close(map.gpsfd);
		map.gpsfd = -1;
		goto bad;
	}
	if(n != Gpsrsize)
		goto bad;
	if(buf[Gpsrsize-1] != '\n')
		goto bad;

	deg = myatol(buf+12);
	min = myatol(buf+17);
	cmin = myatol(buf+20);
	lat = deg + min/60. + cmin/6000.;
	if(buf[23] == 'S')
		lat = -lat;

	deg = myatol(buf+25);
	min = myatol(buf+30);
	cmin = myatol(buf+33);
	lng = deg + min/60. + cmin/6000.;
	if(buf[36] == 'E')
		lng = -lng;

	if(lat < -360 || lat > 360)
		goto bad;
	if(lng < -360 || lng > 360)
		goto bad;

	xx = lat - olat;
	if(xx < -50/6000. || xx > 50/6000.)
		goto sbad;

	xx = lng - olng;
	if(xx < -50/6000. || xx > 50/6000.)
		goto sbad;

	olat = lat;
	olng = lng;
	norm(loc, lat, lng);
	return 1;

sbad:
	olat = lat;
	olng = lng;

bad:
	return 0;
}

void
trackgps(void)
{
	Loc loc, *op, *np;
	long ox, oy, nx, ny;

	if(!map.gpsenabled)
		return;
	if(!getgps(&loc))
		return;
	np = &track[ntrack];
	op = np-1;
	if(ntrack == 0)
		op = &track[nelem(track)-1];

	if(loc.lng == op->lng)
		if(loc.lat == op->lat)
			return;

	ntrack++;
	if(ntrack >= nelem(track))
		ntrack = 0;

	*np = loc;
	ox = op->lng*map.xscale + map.xoffset;
	oy = op->lat*map.yscale + map.yoffset;
	nx = np->lng*map.xscale + map.xoffset;
	ny = np->lat*map.yscale + map.yoffset;

	if(op->lat != 0 && np->lat != 0)
		hiseg(ox,oy, nx,ny, ~Z3, &map.off->r);

	drawmap(loc);
}

void
gpsoff(void)
{
	Loc *np, *op;
	long ox, oy, nx, ny;
	int i;

	if(!map.gpsenabled)
		return;
	np = &track[ntrack];
	nx = np->lng*map.xscale + map.xoffset;
	ny = np->lat*map.yscale + map.yoffset;
	for(i=1; i<nelem(track); i++) {
		op = np;
		ox = nx;
		oy = ny;
		np++;
		if(np >= &track[nelem(track)])
			np = &track[0];
		nx = np->lng*map.xscale + map.xoffset;
		ny = np->lat*map.yscale + map.yoffset;

		if(op->lat != 0 && np->lat != 0)
			hiseg(ox,oy, nx,ny, ~Z3, &map.off->r);
	}
	
}
