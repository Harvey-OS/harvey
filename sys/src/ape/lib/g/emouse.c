#include <stdlib.h>
#include <libg.h>

Mouse
emouse(void)
{
	Mouse m;
	Ebuf *eb;
	static but[2];
	int b;

	if(Smouse < 0)
		berror("events: mouse not initialzed");
	eb = _ebread(&eslave[Smouse]);
	b = eb->buf[1] & 15;
	but[b>>3] = b & 7;
	m.buttons = but[0] | but[1];
	m.xy.x = BGLONG(eb->buf+2);
	m.xy.y = BGLONG(eb->buf+6);
	m.msec = BGLONG(eb->buf+10);
	if (b & 8)		/* window relative */
		m.xy = add(m.xy, screen.r.min);
	free(eb);
	return m;
}

