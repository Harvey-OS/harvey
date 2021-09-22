void parsearguments AA((void));
void error AA((void));
void jumpout AA((void));
void initialize AA((void));
void openinput AA((void));
boolean zinputln AA((textfile f));
#define inputln(f) zinputln((textfile) (f))
void zprintid AA((namepointer p));
#define printid(p) zprintid((namepointer) (p))
void znewxref AA((namepointer p));
#define newxref(p) znewxref((namepointer) (p))
void znewmodxref AA((namepointer p));
#define newmodxref(p) znewmodxref((namepointer) (p))
namepointer zidlookup AA((eightbits t));
#define idlookup(t) zidlookup((eightbits) (t))
namepointer zmodlookup AA((sixteenbits l));
#define modlookup(l) zmodlookup((sixteenbits) (l))
namepointer zprefixlookup AA((sixteenbits l));
#define prefixlookup(l) zprefixlookup((sixteenbits) (l))
boolean linesdontmatch AA((void));
void primethechangebuffer AA((void));
void checkchange AA((void));
void resetinput AA((void));
void getline AA((void));
eightbits zcontrolcode AA((ASCIIcode c));
#define controlcode(c) zcontrolcode((ASCIIcode) (c))
void skiplimbo AA((void));
eightbits skipTeX AA((void));
eightbits zskipcomment AA((eightbits bal));
#define skipcomment(bal) zskipcomment((eightbits) (bal))
eightbits getnext AA((void));
void Pascalxref AA((void));
void outerxref AA((void));
void zmodcheck AA((namepointer p));
#define modcheck(p) zmodcheck((namepointer) (p))
void zflushbuffer AA((eightbits b,boolean percent,boolean carryover));
#define flushbuffer(b, percent, carryover) zflushbuffer((eightbits) (b), (boolean) (percent), (boolean) (carryover))
void finishline AA((void));
void breakout AA((void));
void zoutmod AA((integer m));
#define outmod(m) zoutmod((integer) (m))
void zoutname AA((namepointer p));
#define outname(p) zoutname((namepointer) (p))
void copylimbo AA((void));
eightbits copyTeX AA((void));
eightbits zcopycomment AA((eightbits bal));
#define copycomment(bal) zcopycomment((eightbits) (bal))
void zred AA((sixteenbits j,eightbits k,eightbits c,integer d));
#define red(j, k, c, d) zred((sixteenbits) (j), (eightbits) (k), (eightbits) (c), (integer) (d))
void zsq AA((sixteenbits j,eightbits k,eightbits c,integer d));
#define sq(j, k, c, d) zsq((sixteenbits) (j), (eightbits) (k), (eightbits) (c), (integer) (d))
void fivecases AA((void));
void alphacases AA((void));
textpointer translate AA((void));
void appcomment AA((void));
void appoctal AA((void));
void apphex AA((void));
void easycases AA((void));
void zsubcases AA((namepointer p));
#define subcases(p) zsubcases((namepointer) (p))
void Pascalparse AA((void));
textpointer Pascaltranslate AA((void));
void outerparse AA((void));
void zpushlevel AA((textpointer p));
#define pushlevel(p) zpushlevel((textpointer) (p))
eightbits getoutput AA((void));
void outputPascal AA((void));
void makeoutput AA((void));
void finishPascal AA((void));
void zfootnote AA((sixteenbits flag));
#define footnote(flag) zfootnote((sixteenbits) (flag))
void zunbucket AA((eightbits d));
#define unbucket(d) zunbucket((eightbits) (d))
void zmodprint AA((namepointer p));
#define modprint(p) zmodprint((namepointer) (p))
void PhaseI AA((void));
void PhaseII AA((void));
