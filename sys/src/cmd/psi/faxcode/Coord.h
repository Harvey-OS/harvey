/* Coord.h - defines, typedefs, Inits, empties, and function declarations
   for basic Coordinate geometry data items.
   See Coord.c for associated functions.
   */

/* The basic unit is the scanner pixel, located in the X,Y plane.  As in many
   graphics coordinate systems, X increases left-to-right, while Y increases
   top-down.  Pixel coordinates are usually integer values.  The physical extent
   of integer coordinate `x' along the real X-axis is the half-open interval
   [x,x+1), and similarly for Y.  The actual digitizing resolution (e.g.
   pixels/inch) represented can vary from document to document, and is almost
   always known to the algorithms.  X & Y resolution can be different, though
   for simplicity of discussion we will usually assume that they are equal,
   i.e. that pixels are square.  Thus pixel (x,y) may be thought of as the region
   [x,x+1)X[y,y+1) in the real plane.
	Half-pixel resolution is also supported, with its own data types for
   clarity.  However, to avoid distracting struggles with the type system, they are
   for the most part #defined to be identical to ordinary coordinates.  The
   principle effect is that the limits of scanner coordinates have been halved
   to allow for safe use of half-pixels everywhere.  (This still permits images
   80 inches square at 400 pixels/inch, assuming `short' is at least 16-bit 2's
   complement.)
	Half-pixel coordinates can also be used to describe boundaries of regions,
   as a set of ideal point locations at the corners and edge midpoints of pixels.
   The half-pixel data type is used; boundary points are identified by flags
   in the Bdy "boundary" structure, and may be displayed differently.   */

#define Scoor short	/* Scanner pixel coordinate value */
#define Hcoor Scoor	/* Half-pixel coordinate value */
#define Bcoor Hcoor	/* Half-pixel boundary coordinate value */

/* Each Scoor value `v' is associated of course with two Hcoor values `a' < `b'.
   Conventionally, the physical extent of the `a' half-coordinate is the real
   interval [v,v+0.5), and `b' is [v+0.5,v+1). */
#define StoHa(S) ((S)*2)
#define StoHb(S) ((S)*2+1)
#define HtoS(H) ((H)/2)

/* The 3 half-pixel boundary points of a pixel coordinate `C' are: */
#define StoBa(S) (StoHa((S)))		/* minimum of interval */
#define StoBb(S) (StoHb((S)))		/* midpoint of interval */
#define StoBc(S) (StoHa((S)+1))		/* maximum of interval */

#define Scoor_MIN (SHRT_MIN/2)	/* minimum possible value */
#define Scoor_MAX (SHRT_MAX/2)	/* maximum possible value */

#define Hcoor_MIN (SHRT_MIN)	/* minimum possible value */
#define Hcoor_MAX (SHRT_MAX)	/* maximum possible value */

typedef struct Sp {	/* point: pixel address */
	Scoor x;	/* increases down:  Scoor_MIN is at top */
	Scoor y;	/* increases left-to-right: Scoor_MIN is at left */
	} Sp;
#define Hp Sp		/* Half-pixel point */

#define Init_Zero_Sp {0,0}
#define Init_Min_Sp {Scoor_MIN,Scoor_MIN}
#define Init_Max_Sp {Scoor_MAX,Scoor_MAX}

extern Sp zero_Sp;

/* Is Sp *p1 exactly equal to Sp *p2? */
#define sp_eq(p1,p2) ( \
	((p1)->x == (p2)->x) \
    	 && ((p1)->y == (p2)->y) \
	)

typedef struct Sps {	/* Set of Points */
	int mny;	/* no. points (mny==0 ==> pa==NULL) */
	Sp **pa;	/* NULL-terminated Sp *pa[mny+1] (malloc space)*/
	} Sps;

#define Init_Sps {0,NULL}
extern Sps empty_Sps;

typedef struct Spa {	/* array of Points */
	int mny;	/* no. points in array */
	Sp *a;		/* Sp a[mny] (malloc space)*/
	} Spa;
/** #define Pointa Spa **/  /* OBSOLESCENT */

#define Init_Spa {0,NULL}
extern Spa empty_Spa;

typedef struct {	/* bounding box: inclusive of boundary values */
	Sp a;		/* top-left corner */
	Sp b;		/* bottom-right corner */
	} Bbx;

#define Init_Bbx {Init_Max_Sp,Init_Min_Sp}
#define Init_Max_Bbx {Init_Min_Sp,Init_Max_Sp}
extern Bbx empty_Bbx;
extern Bbx max_Bbx;

/* OBSOLESCENT: */
extern Bbx null_Bbx;

/* height, width, area of Bbx in pixels */
#define bbx_hgt(bxp) ((bxp)->b.y-(bxp)->a.y+1)
#define bbx_wid(bxp) ((bxp)->b.x-(bxp)->a.x+1)
#define bbx_area(bxp) (bbx_hgt((bxp))*bbx_wid((bxp)))

/* Is Bbx *b1 exactly equal to Bbx *b2? */
#define bbx_eq(b1,b2) ( \
	((b1)->a.x == (b2)->a.x) \
    	 && ((b1)->a.y == (b2)->a.y) \
    	 && ((b1)->b.x == (b2)->b.x) \
    	 && ((b1)->b.y == (b2)->b.y) \
	)

/* Is Bbx *b1 wholly inside Bbx *b2? */
#define bbx_inside_all(b1,b2) ( \
	((b1)->a.x >= (b2)->a.x) \
    	 && ((b1)->a.y >= (b2)->a.y) \
    	 && ((b1)->b.x <= (b2)->b.x) \
    	 && ((b1)->b.y <= (b2)->b.y) \
	)

/* Is any of Bbx *b1 inside Bbx *b2? */
#define bbx_inside_any(b1,b2) ( \
	((b1)->a.x <= (b2)->b.x) \
	&& ((b1)->a.y <= (b2)->b.y) \
	&& ((b1)->b.x >= (b2)->a.x) \
	&& ((b1)->b.y >= (b2)->a.y) \
	)

typedef struct Bbxs {	/* A set of Bbxs */
	int mny;	/* if mny==0, then pa==NULL */
	Bbx **pa;	/* NULL-terminated array (in malloc space) of `mny+1'
			   pointers to Bbxs (in malloc space) */
	int alloc;	/* no. slots in pa[] actually allocated (>=mny+1) */
	int incr;	/* no. slots in pa[] to reallocate at a time */
	} Bbxs;

#define Init_Bbxs {0,NULL,0,512}
extern Bbxs empty_Bbxs;

