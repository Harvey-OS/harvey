#include <unistd.h>
#include <libg.h>

Rectangle
bscreenrect(void)
{
	unsigned char buf[18];
	Rectangle r;

	bneed(0);
	if(write(bitbltfd, "i", 1) != 1)
		berror("bscreenrect write /dev/bitblt");
	if(read(bitbltfd, (char *)buf, sizeof buf)!=sizeof buf || buf[0]!='I')
		berror("binit read");
	r.min.x = BGLONG(buf+2);
	r.min.y = BGLONG(buf+6);
	r.max.x = BGLONG(buf+10);
	r.max.y = BGLONG(buf+14);
	return r;
}
