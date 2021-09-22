void parsearguments AA((void));
void initialize AA((void));
void opengffile AA((void));
integer getbyte AA((void));
integer gettwobytes AA((void));
integer getthreebytes AA((void));
integer signedquad AA((void));
void zprintscaled AA((integer s));
#define printscaled(s) zprintscaled((integer) (s))
integer zfirstpar AA((eightbits o));
#define firstpar(o) zfirstpar((eightbits) (o))
boolean dochar AA((void));
void readpostamble AA((void));
