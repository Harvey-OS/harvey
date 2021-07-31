#include <alef.h>
#include <libg.h>

void
cursorswitch(Cursor *c)
{
	byte buf[73];

	bneed(0);
	buf[0] = 'c';
	if(c == nil){
		if(write(bitbltfd, buf, 1) != 1)
			berror("cursorswitch write");
		return;
	}
		
	BPLONG(buf+1, c->offset.x);
	BPLONG(buf+5, c->offset.y);
	memmove(buf+9, c->clr, 2*16);
	memmove(buf+41, c->set, 2*16);
	if(write(bitbltfd, buf, sizeof buf) != sizeof buf)
		berror("cursorswitch write");
}
