void initialize();
void opengffile();
integer getbyte();
integer gettwobytes();
integer getthreebytes();
integer signedquad();
void zprintscaled();
#define printscaled(s) zprintscaled((integer) (s))
integer zfirstpar();
#define firstpar(o) zfirstpar((eightbits) (o))
boolean dochar();
void readpostamble();
