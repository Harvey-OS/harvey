#include "../lib9.h"

#include "../libdraw/draw.h"
#include "../libmemdraw/memdraw.h"

Memcmap*	memdefcmap;

static Memcmap	def;

void
memmkcmap(void)
{

	int i, rgb, r, g, b;

	if(memdefcmap)
		return;

	for(i=0; i<256; i++){
		rgb = cmap2rgb(i);
		r = (rgb>>16)&0xff;
		g = (rgb>>8)&0xff;
		b = rgb&0xff;
		def.cmap2rgb[3*i] = r;
		def.cmap2rgb[3*i+1] = g;
		def.cmap2rgb[3*i+2] = b;
	}

	for(r=0; r<16; r++)
	for(g=0; g<16; g++)
	for(b=0; b<16; b++)
		def.rgb2cmap[r*16*16+g*16+b] = rgb2cmap(r*0x11, g*0x11, b*0x11);
	memdefcmap = &def;
}
