#include <alef.h>
#include <libg.h>

Rectangle
bscreenrect(Rectangle *clipr)
{
	Rectangle r;
	byte buf[34];

	bneed(0);
	if(write(bitbltfd, "i", 1) != 1)
		berror("bscreenrect write /dev/bitblt");
	if(read(bitbltfd, buf, sizeof buf)!=sizeof buf || buf[0]!='I')
		berror("binit read");
	r.min.x = BGLONG(buf+2);
	r.min.y = BGLONG(buf+6);
	r.max.x = BGLONG(buf+10);
	r.max.y = BGLONG(buf+14);
	if(clipr){
		clipr->min.x = BGLONG(buf+18);
		clipr->min.y = BGLONG(buf+22);
		clipr->max.x = BGLONG(buf+26);
		clipr->max.y = BGLONG(buf+30);
	}
	return r;
}
