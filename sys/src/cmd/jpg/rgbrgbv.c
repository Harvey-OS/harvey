#include <u.h>
#include <libc.h>
#include <draw.h>

int
closest(int r, int g, int b)
{
	return rgb2cmap(r, g, b);
}

void
main(int argc, char *argv[])
{
	int i, rgb;
	int r, g, b;
	uchar close[16*16*16];

	/* rgbmap */
	print("uint rgbmap[256] = {\n");
	for(i=0; i<256; i++){
		if(i%8 == 0)
			print("\t");
		rgb = cmap2rgb(i);
		r = (rgb>>16) & 0xFF;
		g = (rgb>>8) & 0xFF;
		b = (rgb>>0) & 0xFF;
		print("0x%.6ulX, ", (r<<16) | (g<<8) | b);
		if(i%8 == 7)
			print("\n");
	}
	print("};\n\n");

	/* closestrgb */
	print("uchar closestrgb[16*16*16] = {\n");
	for(r=0; r<256; r+=16)
	for(g=0; g<256; g+=16)
	for(b=0; b<256; b+=16)
		close[(b/16)+16*((g/16)+16*(r/16))] = closest(r+8, g+8, b+8);
	for(i=0; i<16*16*16; i++){
		if(i%16 == 0)
			print("\t");
		print("%d,", close[i]);
		if(i%16 == 15)
			print("\n");
	}
	print("};\n\n");
	exits(nil);
}
