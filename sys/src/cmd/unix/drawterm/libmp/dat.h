#define	mpdighi  (mpdigit)(1U<<(Dbits-1))
#define DIGITS(x) ((Dbits - 1 + (x))/Dbits)

// for converting between int's and mpint's
#define MAXUINT ((uint)-1)
#define MAXINT (MAXUINT>>1)
#define MININT (MAXINT+1)

// for converting between vlongs's and mpint's
// #define MAXUVLONG (~0ULL)
// #define MAXVLONG (MAXUVLONG>>1)
// #define MINVLONG (MAXVLONG+1ULL)
#define MAXUVLONG ((uvlong) ~0)
#define MAXVLONG (MAXUVLONG>>1)
#define MINVLONG (MAXVLONG+((uvlong) 1))
