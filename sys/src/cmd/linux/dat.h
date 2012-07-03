typedef struct Uproc Uproc;

struct Uproc {
	int		pid;
	int		tid;
	int		*cleartidptr;
};

Uproc uproc;

extern int debug;
