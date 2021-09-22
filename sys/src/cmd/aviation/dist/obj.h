#define	oname	"/lib/aviation/obj"
#define	ONULL	((Obj*)0)
#define	NNAME	10
#ifndef	MYPI
#define	MYPI	(3.14159265359)
#endif
#define	RADIAN	(MYPI/180)

typedef	struct	Obj	Obj;
struct	Obj
{
	char	name[NNAME];
	char	type;
	char	flag;
	float	freq;
	float	lat;
	float	lng;
};

typedef	struct	File	File;
struct	File
{
	char	name[NNAME];
	char	type;
	char	flag;
	char	freq[8];	/* %.2f */
	char	lat[10];	/* %.6f */
	char	lng[10];	/* %.6f */
};

typedef	struct	Mag	Mag;
struct	Mag
{
	float	lat;
	float	lng;
	float	var;
};

typedef	struct	Ft	Ft;
struct	Ft
{
	char	name[10];
	float	lat;
	float	lng;
	short	dist;
	short	hit;
};

extern	double	magvar(Obj*);
extern	double	dist(Obj*, Obj*);
extern	double	angl(Obj*, Obj*);
extern	double	anorm(double);
extern	int	Ofmt(Fmt*);
extern	int	ocmp(char*, Obj*);
extern	int	focmp(char*, File*);
extern	double	xsqrt(double);
extern	void	load(int, char*[], Obj*);
extern	int	mkhere(File*);

#pragma	varargck	type	"O"	Obj*
