#include <u.h>
#include <libc.h>
#include <libg.h>

int
ekbd(void)
{
	Ebuf *eb;
	int c;

	if(Skeyboard < 0)
		berror("events: keyboard not initialzed");
	eb = _ebread(&eslave[Skeyboard]);
	c = eb->buf[0] + (eb->buf[1]<<8);
	free(eb);
	return c;
}
