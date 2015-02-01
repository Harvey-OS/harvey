/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

enum{
	Brdsize 	= 9,
	Psize 		= Brdsize * Brdsize,
	Alldigits 	= 0x1FF,
	Digit 		= 0x0000000F,
	Solve 		= 0x000000F0,
	Allow 		= 0x0001FF00,
	MLock 		= 0x00020000,

	Line 		= 0,
	Thickline 	= 1,
	Border 		= Thickline*4,
	Square 		= 48,
	Maxx 		= Square*9 + 2*Border,
	Maxy 		= Maxx + Square,
};

typedef struct Cell {
	int digit;
	int solve;
	int locked;
} Cell;

Cell	brd[Psize];
Cell	obrd[Psize];
int		board[Psize];

/* game.c */
int getrow(int cell);
int getcol(int cell);
int getbox(int cell);
void setdigit(int cc, int num);
int boxcheck(int *board);
int rowcheck(int *board);
int colcheck(int *board);
int setallowed(int *board, int cc, int num);
int chksolved(int *board);
void attempt(int *pboard, int level);
void clearp(void);
void makep(void);

/* sudoku.c */
void drawbar(int digit, int selected);
void drawcell(int x, int y, int num, Image *col);
void drawblink(int cell);
char *genlevels(int i);

/* levels.c */
void fprettyprintbrd(Cell *board);
void fprintbrd(int fd, Cell *board);
void floadbrd(int fd, Cell *board);
void printboard(Cell *board);
int loadlevel(char *name, Cell *board);
void savegame(Cell *board);
int loadgame(Cell *board);
