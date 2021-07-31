enum{
	Undef = 0x80000000,
	Play = 0,	/* for USB Streaming Terminal K.Okamoto */
	Record = 1,	/* for USB Streaming Terminal K.Okamoto */
	masterRecAGC,	/* feature ID for master record AGC function K.Okamoto */
	masterRecMute,	/* feature IDs for master record mute function K.Okamoto */
	LRRecVol,	/* feature ID for master record L/R volume for Mic, LineIN */
	LRPlayVol,	/* Left/Right volume for speaker, no master volume */
	masterPlayMute,	/* feature ID for master play mute for Speaker, K.Okamoto */
	masterPlayVol,	/* feature ID for master play volume control function */
};

typedef struct Audiocontrol Audiocontrol;

struct Audiocontrol {
	char	*name;
	uchar	readable;
	uchar	settable;
	uchar	chans;		/* 0 is master, non-zero is bitmap */
	long	value[9];	/* 0 is master; value[0] == Undef -> all values Undef */
	long	min, max, step;
};

typedef struct FeatureAttr FeatureAttr;	/* K.Okamoto */

struct FeatureAttr {
	int		id;
	uchar	readable;
	uchar	settable;
	uchar	chans;
	long value[9];
	long	min, max, step;
};

typedef struct MasterVol MasterVol;	/* K.Okamoto */

struct MasterVol {			/* K.Okamoto */
	long	min;
	long max;
	long step;
	long value;		
};

extern Audiocontrol controls[2][Ncontrol];
extern int endpt[2];
extern int interface[2];
extern int featureid[2];		/* current Play/Record Feature ID */
extern int selectorid[2];
extern int selector;			/* K.Okamoto */
extern int mixerid;			/* K.Okamoto */
extern int buttonendpt;

int	ctlparse(char *s, Audiocontrol *c, long *v);
void	ctlevent(void);
int	getspecialcontrol(Audiocontrol *c, int rec, int ctl, int req, long *value);
int	setselector(int);		/* added by K.Okamoto */
int	initmixer(void);	/* added by K.Okamoto */
int	getmasterValue(int, int, int, int, int, long*);	/* added by K.Okamoto */
int	getchannelVol(int, int, int, int, int, uchar, long*);	/* added by K.okamoto */
int	setmasterValue(Audiocontrol*, int, int, int, int, int, long*);	/* K.Okamoto */
int	setchannelVol(Audiocontrol*, int, int, int, int, int, long*);	/* K.Okamoto */
int	printValue(Audiocontrol*, int req, long*);			/* K.Okamoto */

#pragma	varargck	type	"A"	Audiocontrol*
