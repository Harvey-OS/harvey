void parsearguments AA((void));
void initialize AA((void));
void opengffile AA((void));
void openpkfile AA((void));
integer gfbyte AA((void));
integer gfsignedquad AA((void));
void zpkhalfword AA((integer a));
#define pkhalfword(a) zpkhalfword((integer) (a))
void zpkthreebytes AA((integer a));
#define pkthreebytes(a) zpkthreebytes((integer) (a))
void zpkword AA((integer a));
#define pkword(a) zpkword((integer) (a))
void zpknyb AA((integer a));
#define pknyb(a) zpknyb((integer) (a))
integer gflength AA((void));
void zmovetobyte AA((integer n));
#define movetobyte(n) zmovetobyte((integer) (n))
void packandsendcharacter AA((void));
void convertgffile AA((void));
