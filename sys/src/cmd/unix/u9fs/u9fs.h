typedef struct Auth Auth;
struct Auth {
	char *name;

	char* (*auth)(Fcall*, Fcall*);
	char* (*attach)(Fcall*, Fcall*);
	void (*init)(void);
};

extern char remotehostname[];
extern char Eauth[];
extern char *autharg;

extern Auth authrhosts;
extern Auth auth9p1;
extern Auth authnone;

extern ulong truerand(void);
extern void randombytes(uchar*, uint);

extern ulong  msize;
