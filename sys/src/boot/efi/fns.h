extern char hex[];

void usleep(int t);
void jump(void *pc);

int read(void *f, void *data, int len);
int readn(void *f, void *data, int len);
void close(void *f);
void unload(void);

int getc(void);
void putc(int c);

void memset(void *p, int v, int n);
void memmove(void *dst, void *src, int n);
int memcmp(void *src, void *dst, int n);
int strlen(char *s);
char *strchr(char *s, int c);
char *strrchr(char *s, int c);
void print(char *s);

char *configure(void *f, char *path);
char *bootkern(void *f);

char *hexfmt(char *s, int i, uvlong a);
char *decfmt(char *s, int i, ulong a);

long eficall(long narg, void *proc, ...);
void eficonfig(char **cfg);
