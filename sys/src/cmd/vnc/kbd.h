typedef struct	Snarf	Snarf;

struct Snarf
{
	QLock;
	int		vers;
	int		n;
	char		*buf;
};

enum
{
	MAXSNARF	= 100*1024
};

extern	Snarf		snarf;

long			latin1(Rune *k, int n);
void			kbdputc(int c);
void			screenputs(char*, int);
void			vncputc(int, int);
void			setsnarf(char *buf, int n, int *vers);
