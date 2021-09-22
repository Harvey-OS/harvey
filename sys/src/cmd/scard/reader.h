enum {
	TagrdVid =	0x072F,
	TagrdDid =	0x90cc,
	
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

	AOFF = 0,
	AON = 1,
};


char *ttaggetfware(int fd);
int setleds(int fd, uchar leds, uchar t1, uchar t2, uchar n, uchar buzz);
int	antenna(int fd, int ison);
int	autopoll(int fd);
CmdiccR *xchangedata(int fd, uchar *b, int bsz);
CmdiccR *readtag(int fd, int sz);
int	tagrdread(int fd, uchar *buf, long nbytes, vlong offset, Atq *a);
