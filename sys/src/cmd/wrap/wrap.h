/* an open archive, possibly streamed (no seeks) */
typedef struct Ahdr Ahdr;
struct Ahdr {
	char *name;
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

/* wrap management */
typedef struct Update Update;
struct Update {
	char *desc;
	char *dir;
	vlong time;
	vlong utime;
	Biobuf *bmd5;	/* a cache for getfileinfo */
};

typedef struct Wrap Wrap;
struct Wrap {
	char *name;
	char *root;

	Update *u;	/* u[0] is base package, others are patches */
	int nu;
};

Wrap *openwrap(char *name, char *root);
Wrap *openwraphdr(char*, char*, char**, int);
void closewrap(Wrap*);
void Bputwrap(Biobuf*, char*, vlong, char*, vlong);
void Bputwrapfile(Biobuf*, char*, vlong, char*, char*);
int getfileinfo(Wrap*, char*, vlong*, uchar*);

/* one of a kind */
char *mkpath(char *a, char *b);
char *mkpath3(char *a, char *b, char *c);
int waserrstr(void);
int strprefix(char*, char*);
char *readfile(char*);
int opentemp(char*);
int fsort(int fd, char *fil);	/* write sorted file to fd */
vlong filelength(char*);
int match(char*, char**, int);

/* md5 */
int md5file(char*, uchar*);
int md5conv(va_list*, Fconv*);
#pragma	varargck	type	"M"	uchar*

