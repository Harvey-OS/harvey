#include <u.h>
#include <libc.h>
#include <draw.h>
#include <thread.h>
#include <cursor.h>
#include <mouse.h>
#include <keyboard.h>

enum
{
	MAXN = 5
};

typedef struct Piece Piece;
struct Piece{
	short	rot;
	short	tx;
	Point	sz;
	Point	d[MAXN];
};

extern int N, NP;
extern Piece pieces[];

