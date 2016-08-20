/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */


typedef struct	Channel Channel;
typedef struct	Cursor Cursor;
typedef struct	Menu Menu;
typedef struct 	Mousectl Mousectl;

/* as a way to get sort-of anon structs, we make the Mouse and Mousectl structs
 * interchangeable.
 */
struct	Mouse
{
	int	buttons;	/* bit array: LMR=124 */
	Point	xy;
	uint32_t	msec;
};

struct Mousectl
{
	//Mouse;
	int	buttons;	/* bit array: LMR=124 */
	Point	xy;
	uint32_t	msec;
	Channel	*c;	/* chan(Mouse) */
	Channel	*resizec;	/* chan(int)[2] */
			/* buffered in case client is waiting for a mouse action before handling resize */

	char		*file;
	int		mfd;		/* to mouse file */
	int		cfd;		/* to cursor file */
	int		pid;		/* of slave proc */
	Image*	image;	/* of associated window/display */
};

struct Menu
{
	char	**item;
	char	*(*gen)(int);
	int	lasthit;
};

/*
 * Mouse
 */
extern Mousectl*	initmouse(char*, Image*);
extern void		moveto(Mousectl*, Point);
extern int			readmouse(Mousectl*);
extern void		closemouse(Mousectl*);
extern void		setcursor(Mousectl*, Cursor*);
extern void		drawgetrect(Rectangle, int);
extern Rectangle	getrect(int, Mousectl*);
extern int	 		menuhit(int, Mousectl*, Menu*, Screen*);
