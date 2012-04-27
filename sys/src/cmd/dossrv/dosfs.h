enum
{
	Maxfdata	= 8192,
	Maxiosize	= IOHDRSZ+Maxfdata,
};

extern Fcall	*req;
extern Fcall	*rep;
extern char	repdata[Maxfdata];
extern uchar	statbuf[STATMAX];
