typedef struct Glob Glob;
typedef struct Globlist Globlist;

struct Glob{
	String	*glob;
	Glob	*next;
};

struct Globlist{
	Glob	*first;
	Glob	**l;
};

extern	Globlist*	glob(char*);
extern	void		globadd(Globlist*, char*, char*);
extern	void		globlistfree(Globlist *gl);
extern	char*		globiter(Globlist *gl);
