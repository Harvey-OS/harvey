#include <u.h>
#include <libc.h>
#include <thread.h>
#include <draw.h>
#include <keyboard.h>
#include <mouse.h>
#include <control.h>
#include "colors.h"

Font *boldfont;
Font *romanfont;

Image		*background;
Image		*bordercolor;
Image		*black;
Image		*blue;
Image		*darkblue;
Image		*darkgrey;
Image		*darkgreen;
Image		*darkmagenta;
Image		*green;
Image		*grey;
Image		*high;
Image		*land;
Image		*lightblue;
Image		*lightgreen;
Image		*lightgrey;
Image		*lightmagenta;
Image		*low;
Image		*magenta;
Image		*oceanblue;
Image		*pale;
Image		*paleblue;
Image		*paleyellow;
Image		*red;
Image		*sea;
Image		*white;
Image		*yellow;

static ulong
rgba(ulong rgba)
{
	uchar r, g, b, a;

	a = rgba & 0xff;
	b = (rgba >>= 8) & 0xff;
	g = (rgba >>= 8) & 0xff;
	r = (rgba >> 8) & 0xff;
	rgba = ((r * a / 0xff) & 0xff);
	rgba = ((g * a / 0xff) & 0xff) | (rgba << 8);
	rgba = ((b * a / 0xff) & 0xff) | (rgba << 8);
	rgba = (a & 0xff) | (rgba << 8);
	return rgba;
}

void
colorinit(char *roman, char *bold)
{
	Rectangle r = Rect(0, 0, 1, 1);

	white =			display->white;
	black =			display->black;
	blue =			allocimage(display, r, screen->chan, 1, rgba(0x0000ffff));
	darkblue =		allocimage(display, r, screen->chan, 1, rgba(0x0000ccff));
	darkgrey =		allocimage(display, r, screen->chan, 1, rgba(0x444444ff));
	darkgreen =		allocimage(display, r, screen->chan, 1, rgba(0x008800ff));
	darkmagenta =		allocimage(display, r, screen->chan, 1, rgba(0x770077ff));
	green =			allocimage(display, r, screen->chan, 1, rgba(0x00ff00ff));
	grey =			allocimage(display, r, screen->chan, 1, rgba(0x888888ff));
	high =			allocimage(display, r, screen->chan, 1, rgba(0x00ccccff));
	land =			allocimage(display, r, screen->chan, 1, rgba(0xe0ffe0ff));
	lightblue =		allocimage(display, r, screen->chan, 1, rgba(0x88ccccff));
	lightgreen =		allocimage(display, r, screen->chan, 1, rgba(0xaaffaaff));
	lightgrey =		allocimage(display, r, screen->chan, 1, rgba(0xddddddff));
	lightmagenta =		allocimage(display, r, screen->chan, 1, rgba(0xff88ffff));
	low =			allocimage(display, r, screen->chan, 1, rgba(0xddddddff));
	magenta =		allocimage(display, r, screen->chan, 1, rgba(0xbb00bbff));
	oceanblue =		allocimage(display, r, screen->chan, 1, rgba(0x93ddddff));
	pale =			allocimage(display, r, screen->chan, 1, rgba(0xffffaaff));
	paleblue =		allocimage(display, r, screen->chan, 1, rgba(0xddffffff));
	paleyellow =		allocimage(display, r, screen->chan, 1, rgba(0xeeee9eff));
	red =			allocimage(display, r, screen->chan, 1, rgba(0xff0000ff));
	sea =			allocimage(display, r, screen->chan, 1, rgba(0xe0e0ffff));
	yellow =			allocimage(display, r, screen->chan, 1, rgba(0xffff00ff));
	background = sea;
	bordercolor = darkgreen;

	namectlimage(background, "background");
	namectlimage(bordercolor, "border");
	namectlimage(black, "black");
	namectlimage(blue, "blue");
	namectlimage(darkblue, "darkblue");
	namectlimage(darkgreen, "darkgreen");
	namectlimage(darkmagenta, "darkmagenta");
	namectlimage(green, "green");
	namectlimage(grey, "grey");
	namectlimage(high, "high");
	namectlimage(land, "land");
	namectlimage(lightblue, "lightblue");
	namectlimage(lightgreen, "lightgreen");
	namectlimage(lightgrey, "lightgrey");
	namectlimage(lightmagenta, "lightmagenta");
	namectlimage(low, "low");
	namectlimage(magenta, "magenta");
	namectlimage(oceanblue, "oceanblue");
	namectlimage(pale, "pale");
	namectlimage(paleblue, "paleblue");
	namectlimage(paleyellow, "paleyellow");
	namectlimage(red, "red");
	namectlimage(sea, "sea");
	namectlimage(white, "white");
	namectlimage(yellow, "yellow");

	if ((romanfont = openfont(display, roman)) == nil)
		sysfatal("openfont %s: %r", roman);
	namectlfont(romanfont, "romanfont");
	if ((boldfont = openfont(display, bold)) == nil)
		sysfatal("openfont %s: %r", bold);
	namectlfont(boldfont, "boldfont");
}
