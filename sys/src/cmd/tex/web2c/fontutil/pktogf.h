void initialize();
void openpkfile();
void opengffile();
integer getbyte();
integer signedbyte();
integer gettwobytes();
integer signedpair();
integer signedquad();
void zgf16();
#define gf16(i) zgf16((integer) (i))
void zgf24();
#define gf24(i) zgf24((integer) (i))
void zgfquad();
#define gfquad(i) zgfquad((integer) (i))
integer getnyb();
boolean getbit();
integer pkpackednum();
void skipspecials();
