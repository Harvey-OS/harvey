int bzexpand(int);
typedef struct Ramfile	Ramfile;

struct Ramfile {
	char *data;
	int ndata;
};

void *emalloc(ulong);

void ramfs(Tree*, char*, int);
extern int chatty;
