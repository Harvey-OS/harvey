typedef struct Nvfile Nvfile;
typedef struct Fs Fs;

//#include "dosfs.h"
//#include "kfs.h"

struct Nvfile{
	union{
//		Dosfile	dos;
//		Kfsfile	kfs;
		int walked;
	};
	Fs	*fs;
	char	*path;
};

struct Fs{
//	union {
//		Dos dos;
//		Kfs kfs;
//	};
	int	dev;				/* device id */
	long	(*diskread)(Fs*, void*, long);	/* disk read routine */
	Off	(*diskseek)(Fs*, Off);		/* disk seek routine */
	long	(*read)(Nvfile*, void*, long);
	int	(*walk)(Nvfile*, char*);
	Nvfile	root;
};

extern int chatty;
extern int dotini(Fs*);
extern int fswalk(Fs*, char*, Nvfile*);
extern int fsread(Nvfile*, void*, long);
extern int fsboot(Fs*, char*, void*);

#define BADPTR(x) ((ulong)x < 0x80000000)
