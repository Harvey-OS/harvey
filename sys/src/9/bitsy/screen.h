typedef struct Cursorinfo Cursorinfo;
typedef struct Cursor Cursor;

extern ulong blanktime;

struct Cursorinfo {
	Lock;
};

extern void	blankscreen(int);
extern void	flushmemscreen(Rectangle);
extern uchar*	attachscreen(Rectangle*, ulong*, int*, int*, int*);

#define ishwimage(i)	0
