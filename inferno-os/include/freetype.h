/*
 * interface to libfreetype without recourse to the real freetype headers
 * which would otherwise require full-blown CPP
 */

typedef struct FTfaceinfo FTfaceinfo;
struct FTfaceinfo {
	int	nfaces;
	int	index;
	int	style;
	int	height;
	int	ascent;
	char*	familyname;
	char*	stylename;
};

typedef struct FTface FTface;
struct FTface {
	void*	ft_lib;
	void*	ft_face;
};

typedef struct FTglyph FTglyph;
struct FTglyph {
	int	top;
	int	left;
	int	advx;
	int	advy;
	int	height;
	int	width;
	int	bpr;
	uchar*	bitmap;
};

typedef struct FTmatrix FTmatrix;
struct FTmatrix {
	int	a;
	int	b;
	int	c;
	int	d;
};

typedef struct FTvector FTvector;
struct FTvector {
	int	dx;
	int	dy;
};

extern	char*	ftnewface(char *, int, FTface*, FTfaceinfo*);
extern	char*	ftloadmemface(void*, int, int, FTface*, FTfaceinfo*);
extern	char*	ftsetcharsize(FTface, int, int, int, FTfaceinfo*);
extern	void		ftsettransform(FTface, FTmatrix*, FTvector*);
extern	int		fthaschar(FTface,int);
extern	char*	ftloadglyph(FTface, int, FTglyph*);
extern	void		ftdoneface(FTface);
