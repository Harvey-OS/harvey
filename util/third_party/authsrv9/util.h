#define	nil	NULL
#define nelem(p)	(sizeof (p)/sizeof (p)[0])

typedef unsigned char uchar;
/*
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long ulong;
*/
typedef unsigned long long uvlong;
typedef long long vlong;

int vfprint(int fd, char *fmt, va_list ap);
int fprint(int fd, char *fmt, ...);
int print(char *fmt, ...);
long readn(int fd, void *buf, long n);

char *estrdup(char *s);
void randominit(void);
void randombuf(uchar *p, int n);

char *hex(uchar *buf, int n);
int eq(char *a, char *b);
int memeq(void *a, void *b, int n);
int min(int a, int b);

char *remoteaddr(int fd);

enum {
	Deskeylen =	7,
};
void passtokey(uchar *key, char *pw);
void des64key(uchar *k56, uchar *k64);
void authencrypt(uchar *key, uchar *buf, int n);
void authdecrypt(uchar *key, uchar *buf, int n);
