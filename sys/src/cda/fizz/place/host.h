#define		put1(x)		putchar((x))
#define		flush()		(fflush(stdout), fflush(stderr))	/* flush error stuff too */

extern FILE *dbfp;
extern Board b;
extern Rectangle getr(void);
extern Point getp(void);
extern Chip *cofn(int);
extern void sendchip(Chip * , int);
void putl(long n);
void wfile(char *);
void updsig(Chip *);
void putn(int);
void putp(Point );
void putstr(char *);
void rfile(char *);
int rotchip(int);
void sendsig(int);
void sendchsig(int);
void insert(int, int, Point);
void improve(int *, int , int );
int move(int *, Point *, int , Point );
void putr(Rectangle );
void quit(void);
void restartimp(void);

#define	S_CHID		20	/* comfortably above fizz defines */
#define	S_SID		21	/* sig num -> (SIgnal *) */
