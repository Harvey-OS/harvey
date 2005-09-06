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

typedef struct Step {
	uint dir;		/* direction */
	uint count;	/* number of single-step moves */
} Step;

typedef struct Route {
	uint nstep;	/* number of valid Step */
	Step *step;
	Point dest;	/* result of step */
} Route;

typedef struct Walk {
	uint nroute;	/* number of valid Route* */
	Route **route;
	uint beyond;	/* number of allocated Route* */
} Walk;

typedef struct Visited {
	uint 	board[MazeX][MazeY];
} Visited;

typedef struct Animation {
	Route* route;
	Step *step;
	int count;
} Animation;

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
int validpush(Point, Step*, Point*);
int isvalid(Point, Route*, int (*)(Point, Step*, Point*));
void freeroute(Route*);
Route* extend(Route*, int, int, Point);
Route* findroute(Point, Point);

/* animation.c */
void initanimation(Animation*);
void setupanimation(Animation*, Route*);
int onestep(Animation*);
void stopanimation(Animation*);


/* sokoban.c */
char *genlevels(int);
Image *eallocimage(Rectangle, int, uint);
