#include <u.h>
#include <libc.h>
#include <libg.h>

int	npix;
int	nbit;
char	msg[128];
RGB	map[256];

void mapcolor(void), mapgrey(int), mapfile(char *), dumpmap(void);

void
main(int argc, char **argv)
{
	int f;
	char c;
	Bitmap *b;

	binit(0,0,"showimage");
	nbit = (1 << screen.ldepth);
	npix = 1 << nbit;

	ARGBEGIN{
	case 'c':	mapcolor();		break;
	case 'g':	mapgrey(0);		break;
	case 'r':	mapgrey(1);		break;
	case 'm':	mapfile(ARGF());	break;
	case 'd':	dumpmap();		break;
	default:
		fprint(2, "Usage: %s [-cgrd] [-m mapfile] [file ...]\n");
		exits("usage");
	}ARGEND

	for( ;argc > 0; argc--, argv++) {
		f = open(argv[0], OREAD);
		if(f < 0) {
			sprint(msg, "opening file %s", argv[1]);
			perror(msg);
			continue;
		}
		b = rdbitmapfile(f);
		close(f);
		if(!b) {
			perror("reading bitmap");
			continue;
		}
		bitblt(&screen, screen.r.min, b, b->r, S);
		bflush();
		read(0, &c, 1);
		if(c == 'q')
			break;
	}
	exits(0);
}

/* replicate (from top) value in v (n bits) until it fills a ulong */
ulong
rep(ulong v, int n)
{
	int o;
	ulong rv;

	rv = 0;
	for(o = 32 - n; o >= 0; o -= n)
		rv |= (v << o);
	return rv;
}

/*
 * Standard Plan 9 Grey Scale map:
 *  linear from 0 (white) to 1 (black)
 *  or inverse, if rev is 1
 */
void
mapgrey(int rev)
{
	ulong rv;
	int i;

	if(npix > 256)
		return;
	for(i = 0; i < npix; i++){
		rv = rep(i, nbit);
		if(!rev)
			rv = ~rv;
		map[i].red = rv;
		map[i].green = rv;
		map[i].blue = rv;
	}
	wrcolmap(&screen, map);
}

/*
 * Standard Plan 9 Color map (8 bits):
 *   top 3 bits: ~red, next 3 bits: ~green, next 2 bits: ~red
 *   exceptions: pixels 85 and 170 are set to the values they
 *   		have in the grey scale map
 */
void
mapcolor(void)
{
	int i;

	if(npix != 256)
		return;
	for(i = 0; i < npix; i++){
		map[i].red = ~rep((i>>5) & 7, 3);
		map[i].green = ~rep((i>>2) & 7, 3);
		map[i].blue = ~rep(i & 3, 2);
	}
	map[85].red = map[85].green = map[85].blue = 0xB5B5B5B5;	/* was 0xAAAAAAAA */
	map[170].red = map[170].green = map[170].blue = 0x80808080;	/* was 0x55555555 */
	wrcolmap(&screen, map);
}

void
mapfile(char *file)
{
	int i, f, n;
	uchar *p, amap[256*12];

	if(npix > 256)
		return;
	f = open(file, OREAD);
	if(f < 0){
		sprint(msg, "can't open map file %s\n", file);
		perror(msg);
		return;
	}
	n = read(f, amap, npix*12);
	if(n < 0){
		sprint(msg, "can't read map file %s\n", file);
		perror(msg);
		return;
	} else if(n != npix*12){
		fprint(2, "wrong size for map file %s: %d\n", file, n);
		return;
	}
	p = amap;
	for(i = 0; i < npix; i++){
		map[i].red = BGLONG(p);
		map[i].green = BGLONG(p+4);
		map[i].blue = BGLONG(p+8);
		p += 12;
	}
	close(f);
	wrcolmap(&screen, map);
}

void
dumpmap(void)
{
	int i;
	uchar *p, amap[256*12];

	rdcolmap(&screen, map);
	p = amap;
	for(i = 0; i < npix; i++){
		BPLONG(p, map[i].red);
		BPLONG(p+4, map[i].green);
		BPLONG(p+8, map[i].blue);
		p += 12;
	}
	write(1, amap, 12*npix);
}
