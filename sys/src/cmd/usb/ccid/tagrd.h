enum {
	TagrdVid =	0x072f,
	TagrdDid =	0x90cc,

	Scr3310Vid =	0x04e6,
	Scr3310Did =	0x5116,
	
	EndRon = 1<<0,
	EndGon = 1<<1,
	MskR = 1<<2,
	MskG = 1<<3,
	IniRblink = 1<<4,
	IniGblink = 1<<5,
	MskRblink = 1<<6,
	MskGblink = 1<<7,

	NoBuzz = 0x00,
	T1Buzz = 0x01,
	T2Buzz = 0x02,
	TallBuzz = 0x03,
};

int	matchtagrd(char *info);
int	antenna(Ccid *ccid, int ison);
int	autopoll(Ccid *ccid);
Cmsg *xchangedata(Ccid *ccid, uchar *b, int bsz);
Cmsg *readtag(Ccid *ccid, int sz);

extern Iccops tagops;

int	matchscr3310(char *info);