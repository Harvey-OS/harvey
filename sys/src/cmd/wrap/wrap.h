/* an open archive, possibly streamed (no seeks) */
typedef struct Ahdr Ahdr;
struct Ahdr {
	char *name;
	char *modestr;
	Dir;
};

typedef struct Arch Arch;
struct Arch {
	Biobuf *b;
	long nexthdr;
	int canseek;

	Ahdr hdr;
};

Arch *openarch(char*);
Arch *openarchgz(char*, char**, char**, int);
Ahdr *gethdr(Arch*);
void closearch(Arch*);
void Bputhdr(Biobuf*, char*, Dir*);

/* additions to bio(2) */
int Bcopy(Biobuf *dst, Biobuf *src, vlong len);
int Bcopyfile(Biobuf *dst, char *src, vlong len);
char *Bgetline(Biobuf*);
int Bdrain(Biobuf*, vlong);
char *Bsearch(Biobuf*, char*);	/* binary search */

/* open a potentially gzipped file */
Biobuf *Bopengz(char*);

enum {
	FULL = 1<<0,		/* do we have a full package at this point? */
	UPD = 1<<1,		/* is this an update to a previous package? */
};

/* wrap management */
typedef struct Update Update;
struct Update {
	char *desc;		/* textual description */
	char *dir;			/* "/n/kfs/wrap/plan9/12345678" */
	vlong time;		/* time of update */
	vlong utime;		/* time of required prerequisite */
	Biobuf *bmd5;		/* a cache for getfileinfo */
	int type;			/* FULL, UPD, FULL|UPD */
};

typedef struct Wrap Wrap;
struct Wrap {
	char *name;	/* name of package */
	char *root;	/* "/n/kfs" */
	vlong tfull;	/* last time we had all files up-to-date */
	Update *u;	/* sorted by increasing time: u[0] is base package */
	int nu;
};

Wrap *openwrap(char *name, char *root);
Wrap *openwraphdr(char*, char*, char**, int);
void closewrap(Wrap*);
void Bputwrap(Biobuf*, char*, vlong, char*, vlong, int);
void Bputwrapfile(Biobuf*, char*, vlong, char*, char*);
int getfileinfo(Wrap*, char*, vlong*, uchar *tomatch, uchar *towrite);

/* one of a kind */
char *mkpath(char *a, char *b);
char *mkpath3(char *a, char *b, char *c);
int waserrstr(void);
int strprefix(char*, char*);
char *readfile(char*);
int opentemp(char*);
int genopentemp(char*, int, int);
int fsort(int fd, char *fil);	/* write sorted file to fd */
vlong filelength(char*);
int match(char*, char**, int);
void *emalloc(ulong);
void *erealloc(void*, ulong);
char *estrdup(char*);

/* md5 */
int md5file(char*, uchar*);
int md5conv(va_list*, Fconv*);
int Bmd5sum(Biobuf*, uchar*, vlong);
#pragma	varargck	type	"M"	uchar*

