typedef struct Worker Worker;
typedef struct Req Req;
typedef struct Fid Fid;
typedef struct File File;
typedef struct Playlist Playlist;
typedef struct Wmsg Wmsg;
typedef union Pmsg Pmsg;
typedef struct Pacbuf Pacbuf;

enum {
	Qdir,
	Qplayctl,
	Qplaylist,
	Qplayvol,
	Qplaystat,
	Nqid,
};

enum {
	DbgPcm		= 0x01000,
	DbgPac		= 0x02000,
	DbgFs		= 0x10000,
	DbgWorker	= 0x20000,
	DbgPlayer	= 0x40000,
	DbgError	= 0x80000,
};

enum {
	Messagesize = 8*1024+IOHDRSZ,
	Undef = 0x80000000,
	/* 256 buffers of 4096 bytes represents 5.9 seconds
	 * of playout at 44100 Hz (2*16bit samples)
	 */
	NPacbuf = 256,
	Pacbufsize = 4096,
	NSparebuf = 16,	/* For in-line commands (Pause, Resume, Error) */
};

enum {
	/* Named commands (see fs.c): */
	Nostate,	// can't use zero for state
	Error,
	Stop,
	Pause,
	Play,
	Resume,
	Skip,
	/* Unnamed commands */
	Work,
	Check,
	Flush,
	Prep,
	Preq,
};

union Pmsg {
	ulong m;
	struct{
		ushort cmd;
		ushort off;
	};
};

struct Wmsg {
	Pmsg;
	void	*arg;	/* if(cmd != Work) mallocated by sender, freed by receiver */
};

struct Playlist {
	/* The play list consists of a sequence of {objectref, filename}
	 * entries.  Object ref and file name are separated by a tab.
	 * An object ref may not contain a tab.  Entries are seperated
	 * by newline characters.  Neither file names, nor object refs
	 * may contain newlines.
	 */
	ulong	*lines;
	ulong	nlines;
	char	*data;
	ulong	ndata;
};

struct File {
	Dir	dir;
	Channel	*workers;
};

struct Worker
{
	Req	*r;
	Channel	*eventc;
};

struct Fid
{
	int	fid;
	File	*file;
	ushort	flags;
	short	readers;
	ulong	vers;	/* set to file's version when completely read */
	Fid	*next;
};

struct Req
{
	uchar	indata[Messagesize];
	uchar	outdata[Messagesize];
	Fcall	ifcall;
	Fcall	ofcall;
	Fid*	fid;
};

struct Pacbuf {
	Pmsg;
	int len;
	char data[Pacbufsize];
};

void	allocwork(Req*);
Wmsg	waitmsg(Worker*, Channel*);
int	sendmsg(Channel*, Wmsg*);
void	bcastmsg(Channel*, Wmsg*);
void	reqfree(Req*);
Req	*reqalloc(void);
void	readbuf(Req*, void*, long);
void	readstr(Req*, char*);
void	volumeset(int *v);
void	playupdate(Pmsg, char*);
void	playinit(void);
void	volumeproc(void*);
void	srv(void *);
long	robustread(int, void*, long);
void	volumeupdate(int*);
char	*getplaylist(int);
char	*getplaystat(char*, char*);

extern int		debug, aflag;
extern char	*user;
extern Channel	*playc;
extern char	*statetxt[];
extern int		volume[8];
extern Playlist	playlist;
extern Channel	*workers;
extern Channel	*volumechan;
extern Channel	*playchan;
extern Channel	*playlistreq;
extern File	files[];
extern int		srvfd[];
