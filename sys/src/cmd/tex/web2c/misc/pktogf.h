void parsearguments AA((void));
void initialize AA((void));
void openpkfile AA((void));
void opengffile AA((void));
integer getbyte AA((void));
integer signedbyte AA((void));
integer gettwobytes AA((void));
integer signedpair AA((void));
integer signedquad AA((void));
void zgf16 AA((integer i));
#define gf16(i) zgf16((integer) (i))
void zgf24 AA((integer i));
#define gf24(i) zgf24((integer) (i))
void zgfquad AA((integer i));
#define gfquad(i) zgfquad((integer) (i))
integer getnyb AA((void));
boolean getbit AA((void));
integer pkpackednum AA((void));
void skipspecials AA((void));
