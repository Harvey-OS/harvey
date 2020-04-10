/* devmouse.c */
typedef struct Cursor Cursor;
extern Cursor cursor;
extern void mousetrack(int, int, int, ulong);
extern void absmousetrack(int, int, int, ulong);
extern Point mousexy(void);

extern void mouseaccelerate(int);
extern int m3mouseputc(Queue*, int);
extern int m5mouseputc(Queue*, int);
extern int mouseputc(Queue*, int);

/* mouse.c */
extern void mousectl(Cmdbuf*);
extern void mouseresize(void);

/* screen.c */
extern void	blankscreen(int);
extern void	flushmemscreen(Rectangle);
extern uchar*	attachscreen(Rectangle*, ulong*, int*, int*, int*);
extern int	cursoron(int);
extern void	cursoroff(int);
extern void	setcursor(Cursor*);

/* devdraw.c */
extern QLock	drawlock;

#define ishwimage(i)	1		/* for ../port/devdraw.c */

/* swcursor.c */
void		swcursorhide(int);
void		swcursoravoid(Rectangle);
void		swcursordraw(Point);
void		swcursorload(Cursor *);
void		swcursorinit(void);
