void initialize();
void opendvifile();
void opentfmfile();
void readtfmword();
integer getbyte();
integer signedbyte();
integer gettwobytes();
integer signedpair();
integer getthreebytes();
integer signedtrio();
integer signedquad();
integer dvilength();
void zmovetobyte();
#define movetobyte(n) zmovetobyte((integer) (n))
void zprintfont();
#define printfont(f) zprintfont((integer) (f))
boolean zinTFM();
#define inTFM(z) zinTFM((integer) (z))
boolean startmatch();
void inputln();
integer getinteger();
void dialog();
void zdefinefont();
#define definefont(e) zdefinefont((integer) (e))
void flushtext();
void zouttext();
#define outtext(c) zouttext((ASCIIcode) (c))
integer zfirstpar();
#define firstpar(o) zfirstpar((eightbits) (o))
integer zrulepixels();
#define rulepixels(x) zrulepixels((integer) (x))
boolean zspecialcases();
#define specialcases(o, p, a) zspecialcases((eightbits) (o), (integer) (p), (integer) (a))
boolean dopage();
void skippages();
void readpostamble();
