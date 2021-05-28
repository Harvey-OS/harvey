typedef struct SmPath SmPath;
typedef struct TapSm TapSm;
typedef struct Tap Tap;

enum{
	TapReset,
	TapIdle,
	TapSelDR,
	TapCaptureDR,
	TapShiftDR,
	TapExit1DR,
	TapPauseDR,
	TapExit2DR,
	TapUpdateDR,
	TapSelIR,
	TapCaptureIR,
	TapShiftIR,
	TapExit1IR,
	TapPauseIR,
	TapExit2IR,
	TapUpdateIR,
	NStates,
	TapUnknown,
};


enum {
	TapDR,
	TapIR = TapSelIR-TapSelDR,
};

struct TapSm{
	int state;
};

struct SmPath{
	ulong ptms;	/* msb order on the lsb side, see comment on msb2lsb */
	ulong ptmslen;
	int st;
};

struct Tap {
	TapSm;
	u32int hwid;
	int irlen;
	int drlen;
	char name[32];
	void *private;
};

extern void	moveto(TapSm *sm, int dest);
extern SmPath	pathto(TapSm *sm, int dest);
extern SmPath	takepathpref(SmPath *pth, int nbits);
