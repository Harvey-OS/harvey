
typedef struct Group Group;

struct Group {
	Control;
	int		lastbut;
	int		border;
	int		mansize;		/* size was set manually */
	int		separation;
	int		selected;
	int		lastkid;
	CImage	*bordercolor;
	CImage	*image;
	int		nkids;
	Control	**kids;		/* mallocated */
	Rectangle	*separators;	/* mallocated */
	int		nseparators;
};
