#include <u.h>
#include <libc.h>
#include <draw.h>

/*
 * int getcmap(int id, char *file, unsigned char *buf)
 *	Read a colormap from the given file into the buffer.
 *	Returns 1 on success, 0 otherwise.
 *	Goes to unglaublich length to figure out what the file name means:
 *	If the name is "screen" or "display" or "vga", reads the colormap from /dev/draw/id/colormap.
 *	If the name is "gamma",returns gamma=2.3 colormap
 *	If the name is "gamma###", returns gamma=#### colormap
 *	Ditto for rgamma, for reverse video.
 *	Looks for the file in a list of directories (given below).
 */

char *cmapdir[] = {
	"",
	"/lib/cmap/",
	0
};

int
getcmap(int id, char *f, unsigned char *buf)
{
	char name[512];
	char *s, *lines[256], *fields[4];
	int cmap, i, j, n, v, rev;
	double gamma;

	cmap = -1;
	for(i=0; cmapdir[i]!=nil ;i++){
		snprint(name, sizeof name, "%s%s", cmapdir[i], f);
		if((cmap = open(name, OREAD)) >= 0)
			break;
	}

	if(cmap == -1){
		if(strcmp(name, "screen")==0 || strcmp(name, "display")==0 || strcmp(name, "vga")==0){
			snprint(name, sizeof name, "/dev/draw/%d/colormap", id);
			cmap = open(name, OREAD);
			if(cmap < 0)
				return 0;
		}
	}

	if(cmap==-1){ /* could be gamma or gamma<number> or fb */
		if(strncmp(f, "gamma", 5)==0){
			rev=0;
			f+=5;
		}else if(strncmp(f, "rgamma", 6)==0){
			rev = 1;
			f+=6;
		}else
			return 0;
		if(*f == '\0')
			gamma=2.3;
		else{
			if(strspn(f, "0123456789.") != strlen(f))
				return 0;
			gamma = atof(f);
		}
		for(i=0; i!=256; i++){
			v=255.*pow(i/255., 1./gamma);
			if(rev)
				v=255-v;
			buf[0] = buf[1] = buf[2] = v;
			buf += 3;
		}
		return 1;
	}

	s = malloc(20000);
	n = readn(cmap, s, 20000-1);
	if(n <= 0)
		return 0;
	s[n] = '\0';
	if(getfields(s, lines, 256, 0, "\n") != 256)
		return 0;
	for(i=0; i<256; i++){
		if(getfields(lines[i], fields, 4, 1, " \t") != 4)
			return 0;
		if(atoi(fields[0]) != i)
			return 0;
		for(j=0; j<3; j++)
			buf[3*i+j] = atoi(fields[j+1]);
	}
	return 1;
}

/* replicate (from top) value in v (n bits) until it fills a ulong */
ulong
rep(ulong v, int n)
{
	int o;
	ulong rv;

	rv = 0;
	for(o=32-n; o>=0; o-=n)
		rv |= v<<o;
	if(o != -n)
		rv |= v>>-o;
	return rv;
}

void
putcmap(int id, uchar cmap[256*3])
{
	char *s, *t;
	int i, fd;
	char name[64];

	snprint(name, sizeof name, "/dev/draw/%d/colormap", id);
	fd = open(name, OWRITE);
	if(fd < 0)
		sysfatal("can't open colormap file: %r");
	s = malloc(20000);
	t = s;
	for(i = 0; i<256; i++)
		t += sprint(t, "%d %d %d %d\n", i, cmap[3*i+0], cmap[3*i+1], cmap[3*i+2]);
	if(write(fd, s, t-s) != t-s)
		sysfatal("writing color map: %r");
	close(fd);
}

void
main(int argc, char *argv[])
{
	uchar cmapbuf[256*3];
	char *map, buf[12*12+1];
	int fd, id;

	if(argc>2){
		fprint(2, "Usage: %s colormap\n", argv[0]);
		exits("usage");
	}
	map = "rgbv";
	if(argc > 1)
		map = argv[1];

	fd = open("/dev/draw/new", OREAD);
	if(fd < 0 || read(fd, buf, sizeof buf) != 12*12)
		sysfatal("can't connect to display: %r");
	id = atoi(buf+0*12);
	if(strncmp(buf+2*12, "         m8 ", 12) != 0)
		sysfatal("can't set colormap except on CMAP8 (m8) displays; this one is %.12s", buf+2*12);

	if(getcmap(id, map, cmapbuf) == 0){
		fprint(2, "%s: can't find %s\n", argv[0], map);
		exits("not found");
	}

	putcmap(id, cmapbuf);
	exits(0);
}
