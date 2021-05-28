struct File{
	int	walked;
	Fs	*fs;
	char	*path;
};

struct Fs{
	int	dev;				/* device id */
	long	(*diskread)(Fs*, void*, long);	/* disk read routine */
	vlong	(*diskseek)(Fs*, vlong);	/* disk seek routine */
	long	(*read)(File*, void*, long);
	int	(*walk)(File*, char*);
	File	root;
};

extern int chatty;
extern int dotini(Fs*);
extern int fswalk(Fs*, char*, File*);
extern int fsread(File*, void*, long);
extern int fsboot(Fs*, char*, Boot*);

#define BADPTR(x) (0 && (ulong)x < 0x80000000)
