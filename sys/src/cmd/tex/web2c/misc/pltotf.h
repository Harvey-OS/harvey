void parsearguments AA((void));
void initialize AA((void));
void showerrorcontext AA((void));
void fillbuffer AA((void));
void getkeywordchar AA((void));
void getnext AA((void));
void skiptoendofitem AA((void));
void finishtheproperty AA((void));
void lookup AA((void));
void zentername AA((byte v));
#define entername(v) zentername((byte) (v))
void getname AA((void));
byte getbyte AA((void));
void getfourbytes AA((void));
fixword getfix AA((void));
pointer zsortin AA((pointer h,fixword d));
#define sortin(h, d) zsortin((pointer) (h), (fixword) (d))
integer zmincover AA((pointer h,fixword d));
#define mincover(h, d) zmincover((pointer) (h), (fixword) (d))
fixword zshorten AA((pointer h,integer m));
#define shorten(h, m) zshorten((pointer) (h), (integer) (m))
void zsetindices AA((pointer h,fixword d));
#define setindices(h, d) zsetindices((pointer) (h), (fixword) (d))
void junkerror AA((void));
void zreadfourbytes AA((headerindex l));
#define readfourbytes(l) zreadfourbytes((headerindex) (l))
void zreadBCPL AA((headerindex l,byte n));
#define readBCPL(l, n) zreadBCPL((headerindex) (l), (byte) (n))
void zchecktag AA((byte c));
#define checktag(c) zchecktag((byte) (c))
void zprintoctal AA((byte c));
#define printoctal(c) zprintoctal((byte) (c))
boolean zhashinput AA((indx p,indx c));
#define hashinput(p, c) zhashinput((indx) (p), (indx) (c))
indx zf AA((indx h,indx x,indx y));
#define f(h, x, y) zf((indx) (h), (indx) (x), (indx) (y))
indx zeval AA((indx x,indx y));
#define eval(x, y) zeval((indx) (x), (indx) (y))
indx zf AA((indx h,indx x,indx y));
#define f(h, x, y) zf((indx) (h), (indx) (x), (indx) (y))
void zoutscaled AA((fixword x));
#define outscaled(x) zoutscaled((fixword) (x))
void paramenter AA((void));
void nameenter AA((void));
void readligkern AA((void));
void readcharinfo AA((void));
void readinput AA((void));
void corrandcheck AA((void));
