typedef struct Auth Auth;
struct Auth {
	char *name;

	char* (*auth)(Fcall*, Fcall*);
	char* (*attach)(Fcall*, Fcall*);
	void (*init)(void);
	char* (*read)(Fcall*, Fcall*);
	char* (*write)(Fcall*, Fcall*);
	char* (*clunk)(Fcall*, Fcall*);
};

extern char remotehostname[];
extern char Eauth[];
extern char *autharg;

extern Auth authp9any;
extern Auth authrhosts;
extern Auth authnone;

extern ulong truerand(void);
extern void randombytes(uchar*, uint);

extern ulong  msize;

typedef struct Fid Fid;
Fid *newauthfid(int fid, void *magic, char **ep);
Fid *oldauthfid(int fid, void **magic, char **ep);

void safecpy(char *to, char *from, int len);
