typedef struct Symtab
{
	short space;
	char *name;
	void *value;
	struct Symtab *next;
} Symtab;
extern void *symlook(char *, int, void (*)(void));
