/* Keywords */

typedef enum {
	Category,
	Cddata,
	Cmd,
	File,
	Include,
	Key,
	Lyrics,
	Part,
	Path,
	Performance,
	Recording,
	Root,
	Search,
	Soloists,
	Time,
	Track,
	Work,
	Ntoken,	/* Initializer for ntoken */
	Eof	=	-1,
	Txt	=	-2,
	BraceO	=	-3,
	BraceC	=	-4,
	Equals	=	-5,
	Newcat	=	-6,
} Type;

typedef struct Object Object;
typedef struct Catset Catset;
typedef struct Token Token;
typedef struct Cmdlist Cmdlist;

/* Token-only types */

typedef enum {
	Obj,
	Cat,
} Kind;

struct Catset {
	uchar *bitpiece;	/* mallocated */
	int nbitpiece;
};


struct Token {
	char	*name;
	Kind	kind;
	long	value;
	char	*kname;
	Catset	categories;
};

typedef enum {
	Hierarchy,
	Typelist,
	Nlisttype,
} Listtype;

struct Cmdlist {
	int	flag;
	char	*name;
};

#define KEYLEN 128

struct Object {
	Type	type;
	int	tabno;		/* entry in object table */
	Object	*parent;
	Object	**children;	/* mallocated */
	Object	**catparents;
	Object	*orig;		/* back pointer from search object */
	int	nchildren;
	int	ncatparents;
	Catset	categories;	/* was int */
	int	flags;
	int	num;		/* for enumerations */
	char	*value;		/* mallocated */
	char	key[KEYLEN];
	char	*path;		/* mallocated */
};

#define Sort	0x01
#define Enum	0x02
#define Hier	0x04
#define Elab	0x10	/* elaborated rune string */

extern	Token	*tokenlist;
extern	int	ncat;
extern	Object	**catobjects;
extern	Biobuf	*f;
extern	char	*file;
extern	Object	*root;
extern	int	ntoken;

extern	Object	**otab;	// object table
extern	int	notab;	// no of entries used
extern	int	sotab;	// no of entries mallocated
extern	int	hotab;	// no of holes in tab
extern	char	*user;

void	io(void *);
long	printchildren(char*, int, Object*);
long	printdigest(char*, int, Object*);
long	printfiles(char*, int, Object*);
long	printfulltext(char*, int, Object*);
long	printkey(char*, int, Object*);
long	printminiparentage(char*, int, Object*);
long	printparent(char*, int, Object*);
long	printparentage(char*, int, Object*);
long	printtext(char*, int, Object*);
long	printtype(char*, int, Object*);
void	reread(void);
void	listfiles(Object *o);
