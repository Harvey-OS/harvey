void initialize();
void openpkfile();
integer getbyte();
integer gettwobytes();
integer signedquad();
integer getnyb();
boolean getbit();
void zsendout();
#define sendout(repeatcount, value) zsendout((boolean) (repeatcount), (integer) (value))
integer pkpackednum();
void skipspecials();
