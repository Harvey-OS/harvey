/*
 * A polygon outline typeface file contains a sequence of
 * character descriptions.  Each character description
 * starts with a 2 byte word with CHAR (defined below)
 * in the upper 4 bits and a character number in the bottom
 * 12 bits. Following the character number is a sequence of
 * boundaries.  Each boundary begins with a word containing
 * BOUNDARY in the upper 4 bits and the number of vertices
 * in the boundary in the bottom 12 bits.  Following that are
 * pairs of words giving the (integer) x,y coordinates of the
 * points on the boundary.  Words are two bytes, stored low byte
 * first.
 *
 * The interior of a character is to the right of its boundaries.
 * That is, clockwise boundaries surround pieces of the character
 * and counterclockwise boundaries surround holes.
 *
 * Character numbers usually follow the Mergenthaler scheme:
 *   x=	0  1  2  3  4  5  6  7  8  9
 *  ---	----------------------------
 *  0x|	   a  b  c  d  e  f  g  h  i
 *  1x|	j  k  l  m  n  o  p  q  r  s
 *  2x|	t  u  v  w  x  y  z  A  B  C
 *  3x|	D  E  F  G  H  I  J  K  L  M
 *  4x|	N  O  P  Q  R  S  T  U  V  W
 *  5x|	X  Y  Z  &  1  2  3  4  5  6
 *  6x|	7  8  9  0  .  ,  :  ;  ?  !
 *  7x|	(  )  hy '  `  em en $  ce %
 *  8x|	/  di st bu [  ]  *  sc dg dd
 *  9x|	sv ac gr ci .. ti ic cu cd hh
 * 10x| fl fr dl dr pm oe OE iq ie sf
 * 11x|	ld rd bd ss ae AE oa OA o/ O/
 * 12x|	fi fl
 * Abbreviations:
 * .. umlaut, diaresis		AE AE ligature
 * O/ slashed O			OA circle A
 * ac acute accent		ae ae ligature
 * bd baseline double quote	bu bullet
 * cd cedilla			ce cent sign
 * ci circumflex accent		cu cup (high accent)
 * dd double dagger		dg dagger
 * di diagonal			em em dash
 * en en dash			gr grave accent
 * hh high horizontal		hy hyphen
 * ic inverted circumflex	ie inverted exclamation mark
 * iq inverted question mark	ld left double quote
 * o/ slashed o			oa circle a
 * rd right double quote	sc section
 * sf script f			st pound sterling
 * sv short vertical		ti tilde (high, Spanish accent)
 * pm per mil
 */
#define	CMD	0xf000
#define	CHRNO	0x0fff
#define	CHR	0x1000
#define	BOUNDARY	0x2000
#define	NCHR	(CHRNO+1)
#define	NAMES	"  a b c d e f g h i j k l m n o p q r s t u v w x y z A B C D E F G H I J K L M N O P Q R S T U V W X Y Z & 1 2 3 4 5 6 7 8 9 0 . , : ; ? ! ( ) hy' ` emen$ ce% / distbu[ ] * scdgddsvacgrci..tiiccucdhhflfrdldrpmoeOEiqiesfldrdbdssaeAEoaOAo/O/fifl"
/*
 * Incore font outline structures.
 * A Typeface points to an array called glyph of nglyph Glyphs.
 * A Glyph points to an array called boundary of nboundary Boundarys.
 * A Boundary points to an array called pt of npt Points.
 */
typedef struct Typeface Typeface;
typedef struct Glyph Glyph;
typedef struct Boundary Boundary;
struct Typeface{
	int nglyph;
	Glyph *glyph;
};
struct Glyph{
	int nboundary;
	Boundary *boundary;
};
struct Boundary{
	int npt;
	Point *pt;
};
