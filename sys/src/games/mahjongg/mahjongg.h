enum {
	/*
	 * D[1-4], the seasons, appear only once
	 * F[1-4], the flowers, appear only once
	 * everything else appears 4 times
	 * for a total of 144
	 */
	A1 = 0, A2, A3, A4, A5, A6, A7, A8, A9,
	B1, B2, B3, B4, B5, B6, B7, B8, B9,
	C1, C2, C3, C4, C5, C6, C7, C8, C9,
	D1, D2, D3, D4, E1, E2, E3, E4,
	F1, F2, F3, F4, G1, G2, G3,
	Seasons,
	Flowers,
};

enum {
	/* level-specific enums */
	Tiles = 144,
	Depth = 5,
	TileDxy = 6,	/* tile displacement when on a higher level */
	Lx = 32,
	Ly = 16,
	Bord = Depth*TileDxy,
};
enum {
	/* the size of a complete tile */
	Tilex = 60,
	Tiley = 74,

	/* only the face part */
	Facex = 54,
	Facey = 68,

	/* and the entire window, giving room for 5*6 = 30 pixels
	 * that are needed for the higher tiles
	 */
	Sizex = Lx*Facex/2 + 2*Bord,
	Sizey = Ly*Facey/2 + 2*Bord,
};

/* which part of a tile */
typedef enum {
	None,
	TL,			/* main brick */
	TR,
	BR,
	BL,
} Which;

typedef struct {
	Point	start;		/* where is this brick in the tileset */
	int	clicked;
	Which	which;
	int	type;
	int	redraw;
} Brick;

typedef struct {
	int	d;
	int	x;
	int	y;
} Click;

typedef struct {
	Brick 	board[Depth][Lx][Ly];	/* grid of quarter tiles */
	Click	c; 		/* player has a brick selected */
	Click	l; 		/* mouse-over-brick indicator */
	int	done;
	Click	hist[Tiles];
	int 	remaining;
} Level;

Level level;			/* the level played */
Level orig;			/* same, sans modifications */

Image *img;			/* buffer */

Image *background;
Image *brdr;
Image *gameover;
Image *litbrdr;
Image *mask;
Image *selected;
Image *textcol;
Image *tileset;

/* logic.c */
Click	Cl(int d, int x, int y);
Click	NC;
Brick	*bmatch(Click c);
int	canmove(void);
Click	cmatch(Click c, int dtop);
int	eqcl(Click c1, Click c2);
int	isfree(Click c);

/* graphics.c */
void	clearlevel(void);
void	clicked(Point);
void	deselect(void);
void	done(void);
void	drawlevel(void);
void	hint(void);
void	light(Point);
void	resize(Point);
void	undo(void);

/* mahjongg.c */
Image	*eallocimage(Rectangle, int, uint, uint);
char	*genlevels(int);

/* level.c */
void	generate(uint seed);
int	parse(char *);
