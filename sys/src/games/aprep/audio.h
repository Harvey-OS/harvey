#include	<u.h>
#include	<libc.h>
#include	"runeinf.h"

enum
{
	Ggroup	= 1,
	Gfile,
	Gdupl,
	Gvirt,
	Grat,
	Gref,
	Glink,
	Gtitle,
	Gartist,
	Gmusic,
	Gcut,
	Gtext,
	Ntypes,

	Ssort		= 1<<0,
	Sartist		= 1<<1,
	Stitle		= 1<<2,
	Smusic		= 1<<3,
	Snosplit	= 1<<4,
	Snumber		= 1<<5,
	Swassplit	= 1<<6,
	Sgnext		= 1<<7,
	Soff2		= 1<<8,
	Srecur		= 1<<9,
	Snocut		= 1<<10,
	Snoword		= 1<<11,
	Smark		= 1<<12,
	Srelative	= 1<<13,

	Strsize		= 100,
	Maxgroup	= 40,
	Minstar		= 1,
	Maxstar		= 10,
};

typedef	struct	Group		Group;
typedef	struct	Info		Info;
typedef	struct	Cut		Cut;
typedef	struct	Sym		Sym;
typedef	struct	Symlink		Symlink;
typedef	struct	Cxlist		Cxlist;
typedef	struct	Node		Node;
typedef	struct	String		String;
typedef	struct	Word		Word;


struct	Group
{
	union
	{
		struct
		{
			char*	path;
			char*	file;
			char*	volume;
		};
		struct
		{
			Group*	group;
			union
			{
				Symlink*symlink;
				Sym*	ref;
				Group*	egroup;
			};
		};
	};
	long	offset1;
	long	offset2;
	long	off2str;
	long	pair;
	char*	title;
	Info*	info;
	Cut*	cut;
	Group*	link;
	ushort	setup;		/* tree manipulation flags */
	ushort	count;
	ushort	ordinal;
	uchar	type;
};

struct	Symlink
{
	Sym*	sym;
	Symlink*link;
};

struct	Info
{
	char*	name;
	char*	value;
	Group*	group;
	Info*	link;
};

struct	Cut
{
	Group*	parent;
	char*	title;
	int	cut;
	Cut*	link;
};

struct	Sym
{
	char*	name;
	Group*	label;
	Cxlist*	index;
	Sym*	link;
};

struct	Cxlist
{
	Group*	group;
	union
	{
		Cxlist*	link;
		Info*	info;
		Cut*	cut;
	};
};

struct	String
{
	char*	beg;
	char*	end;
};

struct	Word
{
	union
	{
		Cxlist*	index;
		Sym*	sym;
	};
	long	offset;
};

int	allfile;
int	volflag;
int	dupflag;
int	refflag;
Group*	textlist;
char*	apath;
char*	avolume;
char	buf[1000];
Cxlist*	cxlist;
Cxlist*	ecxlist;
int	error;
Sym*	hash[50023];	// Sym*	hash[20011];
int	lineh;
long	lineno;
Cxlist*	listp;
int	maxgroup;
int	nartist;
long	nstringsp;
int	ntitle;
int	nmusic;
long	aux[10];
long	offset;
long	offsetnone;
int	pass;
Group*	root;
char*	strdot;
char*	strnull;
char*	strartist;
char*	strclass;
char*	strfile;
char*	strdupl;
char*	stringsp;
char*	strnosplit;
char*	strnocut;
char*	strnoword;
char*	strmark;
char*	strvarious;
char*	strvolume;
char*	strtext;
char*	strnumber;
char*	strrelative;
char*	strpath;
char*	strrat;
char*	strref;
char*	strsetup;
char*	strsort;
char*	strtitle;
char*	strmusic;
char*	strvirt;
long	totalmal;
char*	typename[Ntypes];
Word*	wordlist;
long	nwordlist;
Sym*	classes[256];
Word*	word;
int	gargi;
int	gargc;
char**	gargv;

int	titcmp(void*, void*);
int	wlcmp(void*, void*);
char*	xtitle(Cxlist*, int);
void	splitgroup(Group*, int);
void	addtitle(Group*, Group*, int);
int	cxcmp(void*, void*);
int	cxcmp1(void*, void*);
int	cxcmp2(void*, void*);
int	cutcmp(void*, void*);
int	cutcmp1(void*, void*);
int	decodeline(void);
void	diag(char*);
void	draw(void);
void	expand(Group*);
void	gencut(Group*, Group*, int);
void	genpath(Group*);
void	gensplit(Group*);
void	gentitle(Group*);
void	gpath(Group*);
void	gdup(Group*);
Info*	getinfo(Info*, char*);
char*	getstrinfo(Info*, char*);
Info*	info(char*, char*);
void*	mal(long);
char*	artstring(Group*);
char*	titstring(Group*);
Group*	parse(void);
void	patch1(Group*);
void	patch1_5(Group*, Info*);
void	patch2(Group*);
void	patch3(Group*);
void	patch4(Group*);
void	patch5(Group*);
void	prcut(Cut*);
void	prartist(Group*);
void	prgroup(Group*);
void	prinfo(Info*);
void	sortcut(Group*);
void	sortgroup(Group*);
char*	strdict(char*);
Sym*	trim(char*);
Sym*	trim1(char*);
void	unmark(Group*);
void	prpath(Group*);
void	goff2(Group*);
Sym*	symdict(char*);
void	genlist(Group*, char*);
void	genpath(Group*);
void	gettext(Sym*);
void	gentext(void);
void	buildtext(void);
void	buildtitle(void);
void	usort(int (*)(void*, void*), int (*)(void*, void*));

void	buildword(char*);
void	buildclass(char*);
void	buildpath(void);
long	doindex(void);
void	searchinit(void);
Cxlist*	ulsort(Cxlist*);
int	getrune(char**);
int	srchash(char*);

void	openr(char*);
void	openw(char*);
void	closer(void);
void	closew(void);
void	putint1(int);
void	putint2(long);
void	putint4(long);
char*	getline(void);
