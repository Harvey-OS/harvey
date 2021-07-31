typedef uvlong Devbytes, Devsects;

typedef struct Biosdrive Biosdrive;	/* 1 drive -> ndevs */
typedef struct Biosdev Biosdev;

struct Biosdrive {
	int	ndevs;
};
struct Biosdev {
	Devbytes size;
	Devbytes offset;
	uchar	id;
	char	type;
};

int	biosboot(int dev, char *file, Boot *b);
void*	biosgetfspart(int i, char *name, int chatty);
void	biosinitdev(int i, char *name);
int	biosinit(void);
void	biosprintbootdevs(int dev);
void	biosprintdevs(int i);
long	biosread(Fs *fs, void *a, long n);
