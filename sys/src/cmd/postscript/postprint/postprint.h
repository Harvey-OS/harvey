/*
 *
 * Default lines per page, tab stops, and point size.
 *
 */

#define LINESPP		66
#define TABSTOPS	8
#define POINTSIZE	10

/*
 *
 * An array of type Fontmap helps convert font names requested by users into
 * legitimate PostScript names. The array is initialized using FONTMAP, which must
 * end with an entry that has NULL defined as its name field. The only fonts that
 * are guaranteed to work well are the constant width fonts.
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
 * Some of the non-integer functions in postprint.c.
 *
 */

char	*get_font();

