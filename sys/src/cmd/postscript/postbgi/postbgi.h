/*
 *
 * BGI opcodes.
 *
 */

#define BRCHAR		033		/* rotated character mode */
#define BCHAR		034		/* graphical character mode */
#define BGRAPH		035		/* graphical master mode */

#define BSUB		042		/* subroutine definition */
#define BRET		043		/* end of subroutine */
#define BCALL		044		/* subroutine call */

#define BEND		045		/* end page */
#define BERASE		046		/* erase - obsolete */
#define BREP		047		/* repeat */
#define BENDR		050		/* end repeat */

#define BSETX		051		/* set horizontal position */
#define BSETY		052		/* set vertical position */
#define BSETXY		053		/* set horizontal and vertical positions */
#define BINTEN		054		/* intensify - mark current pixel */

#define BVISX		055		/* manhattan vector - change x first */
#define BINVISX		056		/* same as BVISX but nothing drawn */
#define BVISY		057		/* manhattan vector - change y first */
#define BINVISY		060		/* same as BVISY but nothing drawn */

#define BVEC		061		/* arbitrary long vector */
#define BSVEC		062		/* arbitrary short vector */
#define BRECT		063		/* outline rectangle */
#define BPOINT1		064		/* point plot - mode 1 */
#define BPOINT		065		/* point plot - mode 2 */
#define BLINE		066		/* line plot */

#define BCSZ		067		/* set character size */
#define BLTY		070		/* select line type */
#define BARC		071		/* draw circular arc */
#define BFARC		072		/* filled circular arc */
#define BFRECT		073		/* filled rectangle */
#define BRASRECT	074		/* raster rectangle */
#define BCOL		075		/* select color */
#define BFTRAPH		076		/* filled trapezoid */
#define BPAT		077		/* pattern are for filling - no info */

#define BNOISE		0		/* from bad file format */

/*
 *
 * Character size is controlled by the spacing of dots in a 5x7 dot matrix, which
 * by default is set to BGISIZE.
 *
 */

#define BGISIZE		2		/* default character grid spacing */

/*
 *
 * Definitions used to decode the bytes read from a BGI file.
 *
 */

#define CHMASK		0177		/* characters only use 7 bits */
#define DMASK		077		/* data values use lower 6 bits */
#define MSB		0100		/* used to check for data or opcode */
#define SGNB		040		/* sign bit for integers */
#define MSBMAG		037		/* mag of most sig byte in a BGI int */

/*
 *
 * Descriptions of BGI vectors and what's done when they're drawn.
 *
 */

#define X_COORD		0		/* change x next in manhattan vector */
#define Y_COORD		1		/* same but y change comes next */
#define LONGVECTOR	2		/* arbitrary long vector */
#define SHORTVECTOR	3		/* components given in 6 bits */

#define VISIBLE		0		/* really draw the vector */
#define INVISIBLE	1		/* just move the current position */

/*
 *
 * What's done with a closed path.
 *
 */

#define OUTLINE		0		/* outline the defined path */
#define FILL		1		/* fill it in */

/*
 *
 * BGI line style definitions. They're used as an index into the STYLES array,
 * which really belongs in the prologue.
 *
 */

#define SOLID		0
#define DOTTED		1
#define SHORTDASH	2
#define DASH		3
#define LONGDASH	4
#define DOTDASH		5
#define THREEDOT	6

#define STYLES								\
									\
	{								\
	    "[]",							\
	    "[.5 2]",							\
	    "[2 4]",							\
	    "[4 4]",							\
	    "[8 4]",							\
	    "[.5 2 4 2]",						\
	    "[.5 2 .5 2 .5 2 4 2]"					\
	}

/*
 *
 * Three constants used to choose which component (RED, GREEN, or BLUE) we're
 * interested in. BGI colors are specified as a single data byte and pulling a
 * particular  component out of the BGI color byte is handled by procedure
 * get_color().
 *
 */

#define RED		0
#define GREEN		1
#define BLUE		2

/*
 *
 * An array of type Disp is used to save the horizontal and vertical displacements
 * that result after a subroutine has been called. Needed so we can properly adjust
 * our horizontal and vertical positions after a subroutine call. Entries are made
 * immediately after a subroutine is defined and used after the call. Subroutine
 * names are integers that range from 0 to 63 (assigned in the BG file) and the
 * name is used as an index into the Disp array when we save or retrieve the
 * displacement.
 *
 */

typedef struct {
	int	dx;			/* horizontal and */
	int	dy;			/* vertical displacements */
} Disp;

/*
 *
 * An array of type Fontmap helps convert font names requested by users into
 * legitimate PostScript names. The array is initialized using FONTMAP, which must
 * end with and entry that has NULL defined as its name field.
 *
 */

typedef struct {
	char	*name;			/* user's font name */
	char	*val;			/* corresponding PostScript name */
} Fontmap;

#define FONTMAP								\
									\
	{								\
	    "R", "Courier",						\
	    "I", "Courier-Oblique",					\
	    "B", "Courier-Bold",					\
	    "CO", "Courier",						\
	    "CI", "Courier-Oblique",					\
	    "CB", "Courier-Bold",					\
	    "CW", "Courier",						\
	    "PO", "Courier",						\
	    "courier", "Courier",					\
	    "cour", "Courier",						\
	    "co", "Courier",						\
	    NULL, NULL							\
	}

/*
 *
 * Two macros that are useful in processing BGI files:
 *
 *       MAG(A, B)    - Takes bytes A and B which have been read from a BGI file
 *                      and returns the magnitude of the integer represented by
 *                      the two bytes.
 *
 *       LINESPACE(A) - Takes BGI size A and returns the number of address units
 *                      that can be used for a reasonable interline spacing.
 *
 */

#define MAG(A, B)	(((A & MSBMAG) << 6) | (B & DMASK))
#define LINESPACE(A)	(8 * A)

/*
 *
 * Some of the non-integer valued functions in postdmd.c.
 *
 */

char	*get_font();

