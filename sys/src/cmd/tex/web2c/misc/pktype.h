void parsearguments AA((void));
void initialize AA((void));
void openpkfile AA((void));
integer getbyte AA((void));
integer gettwobytes AA((void));
integer signedquad AA((void));
integer getnyb AA((void));
boolean getbit AA((void));
void zsendout AA((boolean repeatcount,integer value));
#define sendout(repeatcount, value) zsendout((boolean) (repeatcount), (integer) (value))
integer pkpackednum AA((void));
void skipspecials AA((void));
