void parsearguments AA((void));
void initialize AA((void));
void opendvifile AA((void));
void opentfmfile AA((void));
void readtfmword AA((void));
integer getbyte AA((void));
integer signedbyte AA((void));
integer gettwobytes AA((void));
integer signedpair AA((void));
integer getthreebytes AA((void));
integer signedtrio AA((void));
integer signedquad AA((void));
integer dvilength AA((void));
void zmovetobyte AA((integer n));
#define movetobyte(n) zmovetobyte((integer) (n))
void zprintfont AA((integer f));
#define printfont(f) zprintfont((integer) (f))
boolean zinTFM AA((integer z));
#define inTFM(z) zinTFM((integer) (z))
boolean startmatch AA((void));
void zdefinefont AA((integer e));
#define definefont(e) zdefinefont((integer) (e))
void flushtext AA((void));
void zouttext AA((ASCIIcode c));
#define outtext(c) zouttext((ASCIIcode) (c))
integer zfirstpar AA((eightbits o));
#define firstpar(o) zfirstpar((eightbits) (o))
integer zrulepixels AA((integer x));
#define rulepixels(x) zrulepixels((integer) (x))
boolean zspecialcases AA((eightbits o,integer p,integer a));
#define specialcases(o, p, a) zspecialcases((eightbits) (o), (integer) (p), (integer) (a))
boolean dopage AA((void));
void scanbop AA((void));
void zskippages AA((boolean bopseen));
#define skippages(bopseen) zskippages((boolean) (bopseen))
void readpostamble AA((void));
