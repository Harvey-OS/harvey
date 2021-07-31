void error();
void jumpout();
void scanargs();
void initialize();
void openinput();
boolean zinputln();
#define inputln(f) zinputln((textfile) (f))
void zprintid();
#define printid(p) zprintid((namepointer) (p))
void znewxref();
#define newxref(p) znewxref((namepointer) (p))
void znewmodxref();
#define newmodxref(p) znewmodxref((namepointer) (p))
namepointer zidlookup();
#define idlookup(t) zidlookup((eightbits) (t))
namepointer zmodlookup();
#define modlookup(l) zmodlookup((sixteenbits) (l))
namepointer zprefixlookup();
#define prefixlookup(l) zprefixlookup((sixteenbits) (l))
boolean linesdontmatch();
void primethechangebuffer();
void checkchange();
void resetinput();
void getline();
eightbits zcontrolcode();
#define controlcode(c) zcontrolcode((ASCIIcode) (c))
void skiplimbo();
eightbits skipTeX();
eightbits zskipcomment();
#define skipcomment(bal) zskipcomment((eightbits) (bal))
eightbits getnext();
void Pascalxref();
void outerxref();
void zmodcheck();
#define modcheck(p) zmodcheck((namepointer) (p))
void zflushbuffer();
#define flushbuffer(b, percent, carryover) zflushbuffer((eightbits) (b), (boolean) (percent), (boolean) (carryover))
void finishline();
void breakout();
void zoutmod();
#define outmod(m) zoutmod((integer) (m))
void zoutname();
#define outname(p) zoutname((namepointer) (p))
void copylimbo();
eightbits copyTeX();
eightbits zcopycomment();
#define copycomment(bal) zcopycomment((eightbits) (bal))
void zred();
#define red(j, k, c, d) zred((sixteenbits) (j), (eightbits) (k), (eightbits) (c), (integer) (d))
void zsq();
#define sq(j, k, c, d) zsq((sixteenbits) (j), (eightbits) (k), (eightbits) (c), (integer) (d))
void fivecases();
void alphacases();
textpointer translate();
void appcomment();
void appoctal();
void apphex();
void easycases();
void zsubcases();
#define subcases(p) zsubcases((namepointer) (p))
void Pascalparse();
textpointer Pascaltranslate();
void outerparse();
void zpushlevel();
#define pushlevel(p) zpushlevel((textpointer) (p))
eightbits getoutput();
void outputPascal();
void makeoutput();
void finishPascal();
void zfootnote();
#define footnote(flag) zfootnote((sixteenbits) (flag))
void zunbucket();
#define unbucket(d) zunbucket((eightbits) (d))
void zmodprint();
#define modprint(p) zmodprint((namepointer) (p))
void PhaseI();
void PhaseII();
