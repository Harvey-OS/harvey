#define MAXX	48
#define MAXY	48
#define Min(a,b)	(((a)<(b))?(a):(b))
#define Max(a,b)	(((a)>=(b))?(a):(b))

typedef struct SRC SRC;
struct SRC
{
	uchar	pix[MAXY][MAXX];
};

void	trail(char *);
void	Border(void);
void	Date(int);
void	dive(SRC *, SRC *);
void	redraw(void);
void	error(char*);
void	geticon(SRC *, char *, char *);
void	incoming(char *);
void	itag(int, char *);
void	message(char *, char *);
void	munge(void);
void	nomessage(void);
void	overwrite(Bitmap *);
void	puticon(char *, char *, char *);
void	restart(char *, int, int);
void	showimage(SRC *, int);
void	start_trail(char *);
void	twirl(SRC *, SRC *);
void	wipe(SRC *, SRC *);
void	sayit(char *);

char	user[NAMELEN];
int	First;
int	Same;
Font	*medifont;
SRC	old;
SRC	new;
Point	Offset;
char	*label;
int	aflag;
int	sflag;
char	realmachine[200];
