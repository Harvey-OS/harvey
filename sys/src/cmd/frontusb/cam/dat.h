typedef struct VFrame VFrame;
typedef struct Cam Cam;
typedef struct Format Format;

struct Format {
	VSUncompressedFormat *desc;
	int nframe;
	VSUncompressedFrame **frame;
};

struct VFrame {
	int n, sz, p;
	uchar *d;
	VFrame *next;
};

struct Cam {
	Dev *dev, *ep;
	Iface *iface;
	VSInputHeader *hdr;
	int nformat;
	Format **format;
	ProbeControl pc;
	Cam *next;
	File *ctlfile, *formatsfile, *videofile, *descfile, *framefile;
	
	int active, abort;
	VFrame *actl;
	VFrame *freel;
	Req *delreq;
	QLock qulock;
	int cvtid;
	int framemode;
};

extern int nunit;
extern VCUnit **unit;
extern Iface **unitif;
