#include "../pc/fns.h"

vlong biossize(uint);
long biossectsz(uint);
void bootloadproc(void *);
void changeconf(char *fmt, ...);
Dir *dirchstat(Chan *chan);
int getstr(char *prompt, char *buf, int size, char *def, int timeout);
int gunzip(uchar*, int, uchar*, int);
void i8042a20(void);
void (*i8237alloc)(void);
void impulse(void);
uintptr mapping(uintptr);
void mkmultiboot(void);
void mmuinit0(void);
long mountfix(Chan *c, uchar *op, long n, long maxn);
void mountrewind(Chan *c);
int mountrockread(Chan *c, uchar *op, long n, long *nn);
Chan *namecopen(char *, int);
void readlsconf(void);
void trimnl(char *s);
void unionrewind(Chan *c);
void warp64(uvlong);

/* boot.c */
int bootpass(Boot *b, void *vbuf, int nbuf);

/* conf.c */
void askbootfile(char *buf, int len, char **bootfp, int secs, char *def);
void addconf(char *fmt, ...);
int dotini(char *inibuf);

/* devether.c */
Chan *etherattach(char *spec);
int parseether(uchar*, char*);

/* fs.c */
typedef struct File File;

/* parts.c */
int readparts(char *);

/* pxe.c */
int bind(char *old, char *new, int flag);
long unmount(char *old, char *new);

/* rand.c */
void srand(long);
int nrand(int);

/* stub.c */
long chdir(char *dir);
Chan *namecopen(char *name, int mode);
Chan *enamecopen(char *name, int mode);
Chan *nameccreate(char *name, int mode);
Chan *enameccreate(char *name, int mode);
int myreadn(Chan *c, void *vp, long n);
int readfile(char *file, void *buf, int len);

long dirpackage(uchar *buf, long ts, Dir **d);

/* libip */
int equivip4(uchar *, uchar *);
int equivip6(uchar *, uchar *);
