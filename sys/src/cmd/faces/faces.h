/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

enum	/* face strings */
{
	Suser,
	Sdomain,
	Sshow,
	Sdigest,
	Nstring
};

enum
{
	Facesize = 48,
};

typedef struct Face		Face;
typedef struct Facefile	Facefile;

struct Face
{
	Image	*bit;		/* unless there's an error, this is file->image */
	Image	*mask;	/* unless there's an error, this is file->mask */
	char		*str[Nstring];
	int		recent;
	ulong	time;
	Tm		tm;
	int		unknown;
	Facefile	*file;
};

/*
 * Loading the files is slow enough on a dial-up line to be worth this trouble
 */
struct Facefile
{
	Image	*image;
	Image	*mask;
	ulong	mtime;
	ulong	rdtime;
	int		ref;
	char		*file;
	Facefile	*next;
};

extern char	date[];
extern char	*maildir;
extern char	**maildirs;
extern int	nmaildirs;

Face*	nextface(void);
void	findbit(Face*);
void	freeface(Face*);
void	initplumb(void);
void	killall(char*);
void	showmail(Face*);
void	delete(char*, char*);
void	freefacefile(Facefile*);
Face*	dirface(char*, char*);
void	resized(void);
int	alreadyseen(char*);
ulong	dirlen(char*);

void	*emalloc(ulong);
void	*erealloc(void*, ulong);
char	*estrdup(char*);
char	*findfile(Face*, char*, char*);
void	addmaildir(char*);
