#include <u.h>
#include <libc.h>
#include <libg.h>
#include <fb.h>

#define MAXFRAMES	1000
int nframes = 0;
int mapsize = 0;

int debug = 0;

uchar *pictures[MAXFRAMES];

int msfd = 0;

long msec(void) {
	char buf[50];
	if (msfd == 0)
		msfd = open("/dev/msec", OREAD);
	else
		seek(msfd, 0, 0);
	read(msfd, buf, sizeof(buf));
	return atol(buf);
}

/*
 * length of a line in the bitmap in bytes.  Code taken from
 * /sys/src/libg/rdbitmapfile.c
 */
ulong
bitmaplinesize(Rectangle r, int ld) {
	long dy, px;
	ulong l, t, n;

	if(ld<0 || ld>3 || r.min.x>r.max.x || r.min.y>r.max.y) {
		fprint(2, "bitmap error\n");
		exits("bitmap error");
	}

	px = 1<<(3-ld);	/* pixels per byte */
	/* set l to number of bytes of data per scan line */
	if(r.min.x >= 0)
		return (r.max.x+px-1)/px - r.min.x/px;
	else {	/* make positive before divide */
		t = (-r.min.x)+px-1;
		t = (t/px)*px;
		return (t+r.max.x+px-1)/px;
	}
}

/*
 * allocate memory and save the bitmap in it.
 */
int
savebitmap(Bitmap *bm) {
	int size = bitmaplinesize(bm->r, bm->ldepth) * Dy(bm->r);

	if (nframes == MAXFRAMES) {
		fprint(2, "More than %d frames\n", MAXFRAMES);
		exits("frames overflow");
	}
	pictures[nframes] = (uchar *)malloc(size);
	if (pictures[nframes] == 0) {
		fprint(2, "Out of memory\n");
		exits("memory");
	}
	rdbitmap(bm, bm->r.min.y, bm->r.max.y, pictures[nframes++]);
}

void
show_usage(void) {
	fprint(2, "flip [-r fps] p1 p2 ...\n");
	exits("usage");
}

main(int argc, char *argv[]){
	PICFILE *f;
	Bitmap *b;
	int rate=0, framedelay;
	long lastdisplay=-1;
	int i;

	ARGBEGIN {
	case 'r':	rate = atof(ARGF());			break;
	case 'D':	debug++;				break;
	default:	show_usage();
	} ARGEND;

	if (rate == 0)
		framedelay = 0;
	else
		framedelay = (1000.0/rate);

	binit(0,0,"flip");
	while (argv[0] != 0) {
		f=picopen_r(argv[0]);
		if(f==0){
			picerror(argv[0]);
			exits("can't open picfile");
		}
		if (b != 0)
			bfree(b);
		b=rdpicfile(f, screen.ldepth);
		if(b==0){
			fprint(2, "%s: no space for bitmap\n", argv[0]);
			exits("no space");
		}
		/*
		 * show the viewer as we set up...
		 */
		bitblt(&screen, div(sub(add(screen.r.min, screen.r.max),
			Pt(PIC_WIDTH(f), PIC_HEIGHT(f))), 2), b, b->r, S);
		bflush();
		savebitmap(b);
		argv++;
	}

	while(1) {
		for (i=0; i<nframes; i++) {
			int delay;
			wrbitmap(b,  b->r.min.y, b->r.max.y, pictures[i]);
			if (rate != 0) {
				delay = lastdisplay + framedelay - msec();
				if (delay > 0) {
					if (delay > framedelay) {
						fprint(2, "long delay: %d %d %d %d\n",
							delay, lastdisplay, framedelay,
							msec());
						delay = framedelay;
					}
					sleep(delay);
				}
			}
			bitblt(&screen, div(sub(add(screen.r.min, screen.r.max),
				Pt(PIC_WIDTH(f), PIC_HEIGHT(f))), 2), b, b->r, S);
			bflush();
			if (rate != 0)
				lastdisplay = msec();
		}
	}
}
