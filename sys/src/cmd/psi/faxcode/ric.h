/* ric.h -  Ricoh IS-30 scanner constants, typedefs, function declarations
   The scanner program `rscan' creates a file (typically >1Mbyte).
   It has an ascii header, terminated by \n\n -- see RIC_hdr for its data fields.
   The rest is binary scanner data. Each `RIC_hdr.bpl' bytes holds one scan-line's
   pixels, 1 bit/pixel.  A `1' bit means black.  The order of the bytes in a line
   is left-to-right across the page.  The low-order bit in a byte is the left-most.
   Conventionally, X-coordinates start at 0, at the left of the page, and
   increase to the right.  Y-coordinates start at 0 at the top of the page,
   and increase down.
   */

#include "Coord.h"

typedef struct {	/* Scanner file header */
	short res_x,res_y;  /* resolution in pixels/inch (x,y may differ) */
	short bpl;	/* bytes per scan line */
	Bbx bx;		/* bounding box (pixel indices 0,1,...) */
	} RIC_hdr;


/* constants, for backward compatibility (will fade away) */
#define S_RESOL 300	/* pixels per inch (both X and Y) */
#define S_BYTES	324	/* data bytes per scan line */
/* max no. pixels per line (8.64 inches / 219.5 mm wide) */
#define S_WID_MAX	2592
/* the following have been estimated by eye from sample pages */
#define S_HGT_MAX	3703	/* approx. max lines (11.7 in / 297.2 mm tall) */
#define S_DIAG_MAX	4521	/* approx. length of diagonal of page in pixels */
