typedef struct Glob Glob;

struct Glob{
	char *glob;
	Glob *next;
};

extern Glob*	glob(char *p);
extern void	globfree(Glob*);
extern void	globadd(char*);
