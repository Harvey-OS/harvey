void initialize();
void showerrorcontext();
void fillbuffer();
void getkeywordchar();
void getnext();
byte gethex();
void skiptoendofitem();
void copytoendofitem();
void finishtheproperty();
void lookup();
void zentername();
#define entername(v) zentername((byte) (v))
void getname();
byte getbyte();
void getfourbytes();
fixword getfix();
pointer zsortin();
#define sortin(h, d) zsortin((pointer) (h), (fixword) (d))
integer zmincover();
#define mincover(h, d) zmincover((pointer) (h), (fixword) (d))
fixword zshorten();
#define shorten(h, m) zshorten((pointer) (h), (integer) (m))
void zsetindices();
#define setindices(h, d) zsetindices((pointer) (h), (fixword) (d))
void junkerror();
void zreadfourbytes();
#define readfourbytes(l) zreadfourbytes((headerindex) (l))
void zreadBCPL();
#define readBCPL(l, n) zreadBCPL((headerindex) (l), (byte) (n))
void zchecktag();
#define checktag(c) zchecktag((byte) (c))
void zvffix();
#define vffix(opcode, x) zvffix((byte) (opcode), (fixword) (x))
void zreadpacket();
#define readpacket(c) zreadpacket((byte) (c))
void zprintoctal();
#define printoctal(c) zprintoctal((byte) (c))
boolean zhashinput();
#define hashinput(p, c) zhashinput((indx) (p), (indx) (c))
indx zf();
#define f(h, x, y) zf((indx) (h), (indx) (x), (indx) (y))
indx zeval();
#define eval(x, y) zeval((indx) (x), (indx) (y))
indx zf();
#define f(h, x, y) zf((indx) (h), (indx) (x), (indx) (y))
void zoutscaled();
#define outscaled(x) zoutscaled((fixword) (x))
void zvoutint();
#define voutint(x) zvoutint((integer) (x))
void paramenter();
void vplenter();
void nameenter();
void readligkern();
void readcharinfo();
void readinput();
void corrandcheck();
void vfoutput();
