void initialize();
void opengffile();
void openpkfile();
integer gfbyte();
integer gfsignedquad();
void zpkhalfword();
#define pkhalfword(a) zpkhalfword((integer) (a))
void zpkthreebytes();
#define pkthreebytes(a) zpkthreebytes((integer) (a))
void zpkword();
#define pkword(a) zpkword((integer) (a))
void zpknyb();
#define pknyb(a) zpknyb((integer) (a))
integer gflength();
void zmovetobyte();
#define movetobyte(n) zmovetobyte((integer) (n))
void packandsendcharacter();
void convertgffile();
