typedef struct JMedium JMedium;
typedef struct ShiftTDesc ShiftTDesc;
typedef struct ShiftRDesc ShiftRDesc;


/* descriptor for a shift if not there are too many parameters */
struct ShiftTDesc {
	int		reg;
	uchar	*buf;
	int		nbits;
	int		op;
};

/* descriptor for a reply to do async replies, timing agh... */
struct ShiftRDesc{
	int	nbyread;	/* total bytes */
	int	nbiprelast;	/* bits of next to last */
	int	nbilast;	/* bits of last */
};

/* this is the interface for the medium */
struct JMedium {
	TapSm;				/* of the tap I am currently addressing */
	int		motherb;
	int		tapcpu;
	Tap		taps[16];
	int		ntaps;
	uchar	currentch;	/* BUG: in tap[tapcpu]->private */
	void		*mdata;
	int		(*regshift)(JMedium *jmed, ShiftTDesc *req, ShiftRDesc *rep);
	int		(*flush)(void *mdata);
	int		(*term)(void *mdata);
	int		(*resets)(void *mdata, int trst, int srst);
	int		(*rdshiftrep)(JMedium *jmed, uchar *buf, ShiftRDesc *rep);
};

enum{
	ShiftMarg		= 4,		/* Max number of extra bytes in response */

	ShiftIn		= 1,
	ShiftOut		= 2,
	ShiftNoCommit	= 4,		/* Normally you want to commit... */
	ShiftPauseOut	= 8,		/* go through pause on the way out */
	ShiftPauseIn	= 16,		/* go through pause on the way in */
	ShiftAsync	= 32,		/* do not read reply synch */
};
extern int	tapshift(JMedium *jmed, ShiftTDesc *req, ShiftRDesc *rep, int tapidx);
