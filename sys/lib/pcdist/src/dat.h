typedef struct Saved Saved;
struct Saved
{
	int 	x;
	int 	y;
	int	wid;
	int	nitems;
	char*	store;
};

#define	DOTDOT		(&fmt+1)
#define	ISDIR		0x80000000L

enum
{
	CGAWIDTH	= 160,
	CGAHEIGHT	= 24,

	NP9INI		= 20,
	NLINES		= 10,
	NWIDTH		= 60,
	WLOCX		= 5,
	WLOCY		= 5,

	MENUATTR	= 7,
	HIGHMENUATTR	= 112,
	NMENU		= 30,
};

typedef struct Menu Menu;
struct Menu
{
	int	choice;
	int	selected;

	char*	items[NMENU+1];
};

void	configure(char*, char*, char*);
void	infobox(int, int, char*[], Saved*);
int	menu(int, int, Menu*, Saved*);
void	restore(Saved*);
void	warn(char*);
void	reboot(int);
void	itsdone(char*);
void	probe(char*[]);
int	getc(void);
void	scsiini(void);
int	docmd(char*av[], char*);
void	fixplan9(char*, char*);
