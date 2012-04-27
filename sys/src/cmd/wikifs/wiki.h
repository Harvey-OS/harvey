typedef struct Map Map;
typedef struct Mapel Mapel;
typedef struct Sub Sub;
typedef struct Wdoc Wdoc;
typedef struct Whist Whist;
typedef struct Wpage Wpage;

enum {
	Tcache = 5,	/* seconds */
	Maxmap = 10*1024*1024,
	Maxfile = 100*1024,
};
enum {
	Wpara,
	Wheading,
	Wbullet,
	Wlink,
	Wman,
	Wplain,
	Wpre,
	Whr,
	Nwtxt,
};

struct Wpage {
	int type;
	char *text;
	int section;	/* Wman */
	char *url;		/* Wlink */
	Wpage *next;
};

struct Whist {
	Ref;
	int n;
	char *title;
	Wdoc *doc;
	int ndoc;
	int current;
};

struct Wdoc {
	char *author;
	char *comment;
	int conflict;
	ulong time;
	Wpage *wtxt;
};

enum {
	Tpage,
	Tedit,
	Tdiff,
	Thistory,
	Tnew,
	Toldpage,
	Twerror,
	Ntemplate,
};

struct Sub {
	char *match;
	char *sub;
};

struct Mapel {
	char *s;
	int n;
};

struct Map {
	Ref;
	Mapel *el;
	int nel;
	ulong t;
	char *buf;
	Qid qid;
};

void *erealloc(void*, ulong);
void *emalloc(ulong);
char *estrdup(char*);
char *estrdupn(char*, int);
char *strcondense(char*, int);
char *strlower(char*);

String *s_appendsub(String*, char*, int, Sub*, int);
String *s_appendlist(String*, ...);
Whist *Brdwhist(Biobuf*);
Wpage *Brdpage(char*(*)(void*,int), void*);

void printpage(Wpage*);
String *pagehtml(String*, Wpage*, int);
String *pagetext(String*, Wpage*, int);
String *tohtml(Whist*, Wdoc*, int);
String *totext(Whist*, Wdoc*, int);
String *doctext(String*, Wdoc*);

Whist *getcurrent(int);
Whist *getcurrentbyname(char*);
Whist *gethistory(int);
void closewhist(Whist*);
int allocnum(char*, int);
void freepage(Wpage*);
int nametonum(char*);
char *numtoname(int);
int writepage(int, ulong, String*, char*);
void voidcache(int);

void closemap(Map*);
void currentmap(int);

extern Map *map;
extern RWLock maplock;
extern char *wikidir;
Biobuf *wBopen(char*, int);
int wopen(char*, int);
int wcreate(char*, int, long);
int waccess(char*, int);
Dir *wdirstat(char*);
int opentemp(char*);
