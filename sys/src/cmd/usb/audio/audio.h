enum {
	master_chan		= 0x00,
	Speed_control		= 0x00,
	/* Items below are  defined by USB standard: */
	Mute_control		= 0x01,
	Volume_control		= 0x02,
	Bass_control		= 0x03,
	Mid_control		= 0x04,
	Treble_control		= 0x05,
	Equalizer_control	= 0x06,
	Agc_control		= 0x07,
	Delay_control		= 0x08,
	Bassboost_control	= 0x09,
	Loudness_control	= 0x0a,
	/* Items below are defined by implementation: */
	Channel_control		= 0x0b,
	Resolution_control	= 0x0c,
	Ncontrol,
	Selector_control	= 0x0d,

	sampling_freq_control	= 0x01,

	Audiocsp = 0x000101, /* audio.control.0 */

	AUDIO_INTERFACE = 0x24,
	AUDIO_ENDPOINT = 0x25,
};


#define AS_GENERAL 1
#define FORMAT_TYPE 2
#define FORMAT_SPECIFIC 3

#define PCM 1
#define PCM8 2
#define IEEE_FLOAT 3
#define ALAW 4
#define MULAW 5

#define SAMPLING_FREQ_CONTROL 0x01

typedef struct Audioalt Audioalt;

struct Audioalt {
	int		nchan;
	int		res;
	int		subframesize;
	int		minfreq, maxfreq;	/* continuous freqs */
	int		freqs[8];		/* discrete freqs */
	int		caps;		/* see below for meanings */
};

enum {
	/* Audioalt->caps bits */
	has_setspeed = 0x1,		/* has a speed_set command */
	has_pitchset = 0x2,		/* has a pitch_set command */
	has_contfreq = 0x4,		/* frequency continuously variable */
	has_discfreq = 0x8,		/* discrete set of frequencies */
	onefreq = 0x10,		/* only one frequency */
	maxpkt_only = 0x80,	/* packets must be padded to max size */
};

typedef uchar byte;

extern int setrec;
extern int verbose;
extern int defaultspeed[2];
extern Dev *ad;
extern Dev *buttondev;
extern Channel *controlchan;
extern Dev *epdev[2];

void	audio_interface(Dev *d, Desc *dd);
void	setalt(Dev *d, int endpt, int value);
int	getalt(Dev *d, int endpt);
int	setspeed(int rec, int speed);
int	setcontrol(int rec, char *name, long *value);
int	getspecialcontrol(int rec, int ctl, int req, long *value);
int	getcontrol(int rec, char *name, long *value);
int	findalt(int rec, int nchan, int res, int speed);
void	getcontrols(void);
void	serve(void *);
int	nbchanprint(Channel *c, char *fmt, ...);
int	Aconv(Fmt *fp);
