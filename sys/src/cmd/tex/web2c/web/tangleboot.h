void error();
void scanargs();
void initialize();
void openinput();
boolean zinputln();
#define inputln(f) zinputln((textfile) (f))
void zprintid();
#define printid(p) zprintid((namepointer) (p))
namepointer zidlookup();
#define idlookup(t) zidlookup((eightbits) (t))
namepointer zmodlookup();
#define modlookup(l) zmodlookup((sixteenbits) (l))
namepointer zprefixlookup();
#define prefixlookup(l) zprefixlookup((sixteenbits) (l))
void zstoretwobytes();
#define storetwobytes(x) zstoretwobytes((sixteenbits) (x))
void zpushlevel();
#define pushlevel(p) zpushlevel((namepointer) (p))
void poplevel();
sixteenbits getoutput();
void flushbuffer();
void zappval();
#define appval(v) zappval((integer) (v))
void zsendout();
#define sendout(t, v) zsendout((eightbits) (t), (sixteenbits) (v))
void zsendsign();
#define sendsign(v) zsendsign((integer) (v))
void zsendval();
#define sendval(v) zsendval((integer) (v))
void sendtheoutput();
boolean linesdontmatch();
void primethechangebuffer();
void checkchange();
void getline();
eightbits zcontrolcode();
#define controlcode(c) zcontrolcode((ASCIIcode) (c))
eightbits skipahead();
void skipcomment();
eightbits getnext();
void zscannumeric();
#define scannumeric(p) zscannumeric((namepointer) (p))
void zscanrepl();
#define scanrepl(t) zscanrepl((eightbits) (t))
void zdefinemacro();
#define definemacro(t) zdefinemacro((eightbits) (t))
void scanmodule();
