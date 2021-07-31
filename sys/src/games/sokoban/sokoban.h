enum {
	/* levels */
	Empty		= 0,
	Background,
	Wall, 
	Cargo,
	Goal,
	GoalCargo,
	Glenda,

	/* movements */
	Up,
	Down,
	Left,
	Right,
};

enum {
	/* glenda faces the horizontal direction she's moving in */
	GLeft	= 0,
	GRight 	= 1,
};

enum {
	MazeX = 20,
	MazeY = 18,
	BoardX = 49,
	BoardY = 49,
	SizeX = MazeX*BoardX+10,	
	SizeY = MazeY*BoardY+10,

	Maxlevels = 200,
};

typedef struct {
	uint dir;		/* direction */
	uint count;	/* number of single-step moves */
} Step;

typedef struct {
	uint nstep;	/* number of valid steps */
	Step *step;
	uint beyond;	/* number of allocated Step */
} Route;

typedef struct {
	uint 	board[MazeX][MazeY];
} Visited;

typedef struct {
	Point 	glenda;
	Point 	max;		/* that's how much the board spans */
	uint 	index;
	uint	done;
	uint 	board[MazeX][MazeY];
} Level;

Level level;		/* the current level */
Level levels[Maxlevels];	/* all levels from this file */
int numlevels;		/* how many levels do we have */
int animate;		/* boolean: animate during multi-step move? */

Image *img;		/* buffer */
Image *text;		/* for text messages */
Image *win;

Image *goal;
Image *cargo;
Image *goalcargo;
Image *wall;
Image *empty;
Image *gleft;
Image *gright;
Image *glenda;
Image *bg;

/* graphics.c */
void drawscreen(void);
void drawlevel(void);
void drawwin(void);
void drawglenda(void);
void drawboard(Point);
void resize(Point);
Point boardsize(Point);

/* level.c */
int loadlevels(char *);

/* move.c */
void move(int);

/* route.c */
Route* newroute(void);
void freeroute(Route*);
void reverseroute(Route*);
void pushstep(Route*, int, int);
void popstep(Route*);
int validwalk(Point, Step, Point*);
int validpush(Point, Step, Point*);
int isvalid(Point, Route*, int (*)(Point, Step, Point*));
int findwalk(Point, Point, Route*);
void applyroute(Route*);

/* sokoban.c */
char *genlevels(int);
Image *eallocimage(Rectangle, int, uint);
