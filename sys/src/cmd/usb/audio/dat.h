typedef struct Audiocontrol Audiocontrol;
typedef struct Audiofunc Audiofunc;
typedef struct Funcalt Funcalt;
typedef struct Nexus Nexus;
typedef struct Stream Stream;
typedef struct Streamalt Streamalt;
typedef struct Unit Unit;

enum
{
	master_chan				= 0x00,
	Speed_control				= 0x00,
	/* Items below are  defined by USB standard: */
	Mute_control				= 0x01,
	Volume_control			= 0x02,
	Bass_control				= 0x03,
	Mid_control				= 0x04,
	Treble_control				= 0x05,
	Equalizer_control			= 0x06,
	Agc_control				= 0x07,
	Delay_control				= 0x08,
	Bassboost_control			= 0x09,
	Loudness_control			= 0x0a,
	/* Items below are define by implementation: */
	Channel_control			= 0x0b,
	Resolution_control			= 0x0c,
	Ncontrol,
	sampling_freq_control		= 0x01,
};

enum
{
	Undef = 0x80000000,
	Play = 0,
	Record = 1,
};

struct Audiocontrol
{
	uchar	readable;
	uchar	settable;
	uchar	chans;		/* 0 is master, non-zero is bitmap */
	long		value[8];		/* 0 is master; value[0] == Undef -> all values Undef */
	long		min, max, step;
};

struct Audiofunc
{
	Dinf			*intf;
	Funcalt		*falt;
	Stream		*streams;	/* cache */
};

struct Funcalt
{
	Dalt			*dalt;		/* interface (alternative) for audiocontrol interface */
	int			nstream;	/* number of additional interfaces in collection */
	Stream		**stream;	/* array of audio and midi streams */
	Unit			*unit;	/* linked list of Units */
	Funcalt		*next;
};

struct Stream
{
	Dinf			*intf;
	int			id;		/* associated terminal id */
	Streamalt		*salt;		/* list of alternates for this stream */
	Stream		*next;	/* next in cache */
};

struct Streamalt
{
	Dalt			*dalt;		/* interface alternative for this stream alternative */
	uchar		unsup;		/* set if format type is unknown/unsupported */
	uchar		delay;
	ushort		format;
	uchar		nchan;
	uchar		subframe;
	uchar		res;
	uchar		attr;
	uchar		lockunits;
	int			lockdelay;
	int			minf;
	int			maxf;
	uchar		nf;
	int			*f;
	Streamalt		*next;	/* in list of alternates */
};

enum
{
	// Streamalt.attr
	Asampfreq = (1<<0),
	Apitch = (1<<1),
	Amaxpkt = (1<<7),

	// Streamalt.lockunits
	Lundef = 0,
	Lmillisec = 1,
	Lsamp = 2,
};

struct Unit
{
	uchar		type;			/* from descriptor subtype */
	uchar		id;
	uchar		associf;		/* # of associated interface */
	uchar		nchan;		/* number of logical channels */
	ushort		chanmask;	/* spatial location of channels */

	int			nsource;		/* number of inputs (sources) */
	int			*sourceid;		/* ids of inputs */
	Unit			**source;		/* pointers to actual Unit structures of inputs */

	union {
		/* for terminals: */
		struct {
			uchar		assoc;		/* id of associated terminal, if any */
			int			termtype;		/* terminal type */
			Stream		*stream;		/* associated stream */
		};

		/* for feature units: */
		struct {
			int			*hascontrol;	/* per-channel bitmasks */
			uchar		*fdesc;		/* saved descriptor for second pass */
		};

		/* for processing units: */
		struct {
			int			proctype;
		};

		/* for extension units: */
		struct {
			int			exttype;
		};
	};
	Unit			*next;		/* in list of all Units/Terminals */
};

struct Nexus
{
	char			*name;
	int			id;
	int			dir;
	Audiofunc	*af;
	Funcalt		*alt;
	Stream		*s;
	Unit			*input;
	Unit			*feat;
	Unit			*output;
	Audiocontrol	control[Ncontrol];
	Nexus		*next;
};

enum
{
	/* Nexus.dir */
	Nin,
	Nout,
};

extern Channel *controlchan;

extern int buttonendpt;
extern Nexus *nexus[2];
extern char *controlname[Ncontrol];

extern int debug;
extern int debugdebug;
extern int verbose;

enum {
	Dbginfo = 0x01,
	Dbgfs = 0x02,
	Dbgproc = 0x04,
	Dbgcontrol = 0x08,
};

#pragma	varargck	type	"A"	Audiocontrol*
