void initialize();
void readtfmword();
integer zvfread();
#define vfread(k) zvfread((integer) (k))
integer ztfmwidth();
#define tfmwidth(c) ztfmwidth((byte) (c))
void zoutdigs();
#define outdigs(j) zoutdigs((integer) (j))
void zprintdigs();
#define printdigs(j) zprintdigs((integer) (j))
void zprintoctal();
#define printoctal(c) zprintoctal((byte) (c))
void outln();
void left();
void right();
void zoutBCPL();
#define outBCPL(k) zoutBCPL((index) (k))
void zoutoctal();
#define outoctal(k, l) zoutoctal((index) (k), (index) (l))
void zoutchar();
#define outchar(c) zoutchar((byte) (c))
void zoutface();
#define outface(k) zoutface((index) (k))
void zoutfix();
#define outfix(k) zoutfix((index) (k))
void zcheckBCPL();
#define checkBCPL(k, l) zcheckBCPL((index) (k), (index) (l))
void hashinput();
index zligf();
#define ligf(h, x, y) zligf((index) (h), (index) (x), (index) (y))
index zeval();
#define eval(x, y) zeval((index) (x), (index) (y))
index zligf();
#define ligf(h, x, y) zligf((index) (h), (index) (x), (index) (y))
boolean zstringbalance();
#define stringbalance(k, l) zstringbalance((integer) (k), (integer) (l))
void zoutasfix();
#define outasfix(x) zoutasfix((integer) (x))
integer zgetbytes();
#define getbytes(k, issigned) zgetbytes((integer) (k), (boolean) (issigned))
boolean vfinput();
boolean organize();
void dosimplethings();
boolean zdomap();
#define domap(c) zdomap((byte) (c))
boolean docharacters();
