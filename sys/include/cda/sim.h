#pragma	lib	"cda/libsim.a"

typedef struct Cell Cell;

struct Cell {
	char *name;
	short period;	/* shows we're clocked */
	short y,oy;	/* waveform */
	short ox;
	uchar m;	/* master */
	uchar s;	/* slave */
	uchar ck;	/* clock */
	uchar rd;	/* reset data */
	uchar pre;	/* preset data */
	uchar ce;	/* clock enable */
	uchar disp;	/* we want to see it */
	uchar trig;	/* trigger value */
	uchar dis;	/* output disable */
	uchar drive;	/* driven input */
	uchar vert;	/* edge to watch */
	uchar prev;	/* previous s value */
};

uchar settle(void);
void init(void);
void binde(char *, Cell *);
