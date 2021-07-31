#pragma src "/sys/src/libdisk"
#pragma lib "libdisk.a"

/* SCSI interface */
typedef struct Scsi Scsi;
struct Scsi {
	QLock;
	char*	inquire;
	int	rawfd;
	int	nchange;
	ulong	changetime;
};

enum {
	Sread = 0,
	Swrite,
	Snone,
};

char*	scsierror(int, int);
int		scsicmd(Scsi*, uchar*, int, void*, int, int);
int		scsi(Scsi*, uchar*, int, void*, int, int);
Scsi*		openscsi(char*);
void		closescsi(Scsi*);
int		scsiready(Scsi*);

extern int		scsiverbose;

/* disk partition interface */
typedef struct Disk Disk;
struct Disk {
	char *prefix;
	char *part;
	int fd;
	int wfd;
	int ctlfd;
	int rdonly;
	int type;

	vlong secs;
	vlong secsize;
	vlong size;
	vlong offset;	/* within larger disk, perhaps */
	int width;	/* of disk size in bytes as decimal string */
	int c;
	int h;
	int s;
	int chssrc;
};

Disk*	opendisk(char*, int, int);

enum {
	Tfile = 0,
	Tsd,
	Tfloppy,

	Gpart = 0,	/* partition info source */
	Gdisk,
	Gguess,
};

/* proto file parsing */
typedef void Protoenum(char *new, char *old, Dir *d, void *a);
typedef void Protowarn(char *msg, void *a);
int rdproto(char*, char*, Protoenum*, Protowarn*, void*);
