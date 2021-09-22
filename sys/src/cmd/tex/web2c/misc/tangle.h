void error AA((void));
void parsearguments AA((void));
void initialize AA((void));
void openinput AA((void));
boolean zinputln AA((textfile f));
#define inputln(f) zinputln((textfile) (f))
void zprintid AA((namepointer p));
#define printid(p) zprintid((namepointer) (p))
namepointer zidlookup AA((eightbits t));
#define idlookup(t) zidlookup((eightbits) (t))
namepointer zmodlookup AA((sixteenbits l));
#define modlookup(l) zmodlookup((sixteenbits) (l))
namepointer zprefixlookup AA((sixteenbits l));
#define prefixlookup(l) zprefixlookup((sixteenbits) (l))
void zstoretwobytes AA((sixteenbits x));
#define storetwobytes(x) zstoretwobytes((sixteenbits) (x))
void zpushlevel AA((namepointer p));
#define pushlevel(p) zpushlevel((namepointer) (p))
void poplevel AA((void));
sixteenbits getoutput AA((void));
void flushbuffer AA((void));
void zappval AA((integer v));
#define appval(v) zappval((integer) (v))
void zsendout AA((eightbits t,sixteenbits v));
#define sendout(t, v) zsendout((eightbits) (t), (sixteenbits) (v))
void zsendsign AA((integer v));
#define sendsign(v) zsendsign((integer) (v))
void zsendval AA((integer v));
#define sendval(v) zsendval((integer) (v))
void sendtheoutput AA((void));
boolean linesdontmatch AA((void));
void primethechangebuffer AA((void));
void checkchange AA((void));
void getline AA((void));
eightbits zcontrolcode AA((ASCIIcode c));
#define controlcode(c) zcontrolcode((ASCIIcode) (c))
eightbits skipahead AA((void));
void skipcomment AA((void));
eightbits getnext AA((void));
void zscannumeric AA((namepointer p));
#define scannumeric(p) zscannumeric((namepointer) (p))
void zscanrepl AA((eightbits t));
#define scanrepl(t) zscanrepl((eightbits) (t))
void zdefinemacro AA((eightbits t));
#define definemacro(t) zdefinemacro((eightbits) (t))
void scanmodule AA((void));
