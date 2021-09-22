void parsearguments AA((void));
void initialize AA((void));
void readtfmword AA((void));
integer zvfread AA((integer k));
#define vfread(k) zvfread((integer) (k))
integer ztfmwidth AA((byte c));
#define tfmwidth(c) ztfmwidth((byte) (c))
void zoutdigs AA((integer j));
#define outdigs(j) zoutdigs((integer) (j))
void zprintdigs AA((integer j));
#define printdigs(j) zprintdigs((integer) (j))
void zprintoctal AA((byte c));
#define printoctal(c) zprintoctal((byte) (c))
void outln AA((void));
void left AA((void));
void right AA((void));
void zoutBCPL AA((indextype k));
#define outBCPL(k) zoutBCPL((indextype) (k))
void zoutoctal AA((indextype k,indextype l));
#define outoctal(k, l) zoutoctal((indextype) (k), (indextype) (l))
void zoutchar AA((byte c));
#define outchar(c) zoutchar((byte) (c))
void zoutface AA((indextype k));
#define outface(k) zoutface((indextype) (k))
void zoutfix AA((indextype k));
#define outfix(k) zoutfix((indextype) (k))
void zcheckBCPL AA((indextype k,indextype l));
#define checkBCPL(k, l) zcheckBCPL((indextype) (k), (indextype) (l))
void hashinput AA((void));
indextype zligf AA((indextype h,indextype x,indextype y));
#define ligf(h, x, y) zligf((indextype) (h), (indextype) (x), (indextype) (y))
indextype zeval AA((indextype x,indextype y));
#define eval(x, y) zeval((indextype) (x), (indextype) (y))
indextype zligf AA((indextype h,indextype x,indextype y));
#define ligf(h, x, y) zligf((indextype) (h), (indextype) (x), (indextype) (y))
boolean zstringbalance AA((integer k,integer l));
#define stringbalance(k, l) zstringbalance((integer) (k), (integer) (l))
void zoutasfix AA((integer x));
#define outasfix(x) zoutasfix((integer) (x))
integer zgetbytes AA((integer k,boolean issigned));
#define getbytes(k, issigned) zgetbytes((integer) (k), (boolean) (issigned))
boolean vfinput AA((void));
boolean organize AA((void));
void dosimplethings AA((void));
boolean zdomap AA((byte c));
#define domap(c) zdomap((byte) (c))
boolean docharacters AA((void));
