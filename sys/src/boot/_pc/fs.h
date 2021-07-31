typedef struct File File;
typedef struct Fs Fs;

#include "dosfs.h"
#include "kfs.h"

struct File{
	union{
		Dosfile	dos;
		Kfsfile	kfs;
		int walked;
	};
	Fs	*fs;
	char	*path;
};

struct Fs{
	union {
		Dos dos;
		Kfs kfs;
	};
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

#define BADPTR(x) ((ulong)x < 0x80000000)
