/*
 * Global data structures too hard to translate automatically from
 * Pascal.  This file is included from the change file in the change for
 * section 8.113.
 */

typedef union {
    struct {
	halfword RH, LH;
    } v;
    struct {
	halfword junk_space;	/* Make B0,B1 overlap LH in memory */
	quarterword B0, B1;
    } u;
} twohalves;
#define	b0	u.B0
#define	b1	u.B1
#define	b2	u.B2
#define	b3	u.B3

typedef struct {
    struct {
	quarterword B0;
	quarterword B1;
	quarterword B2;
	quarterword B3;
    } u;
} fourquarters;

typedef union {
    integer cint;
    glueratio gr;
    twohalves hh;
    fourquarters qqqq;
} memoryword;
