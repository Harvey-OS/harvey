/* Datatypes for simpler access to data structures in TrueType fonts */


/* Font Directory Component */

typedef struct {
	UInt32		tagName;
	UInt32		checkSum;
	UInt32		offset;
	UInt32		length;
} TTFontDirComponent;



/* Font Directory */

typedef struct {
	UInt32				version;
	UInt16				numTables;
	UInt16				searchRange;
	UInt16				entrySelector;
	UInt16				rangeShift;
	TTFontDirComponent	components[1];	/* an array of numTables components */
} TTFontDir;



/* Tag definitions */
#define		TTF_FONT_NAMING_TABLE	'name'



/* Font Naming Table */

typedef struct {
	UInt16				formatSelector;
	UInt16				numNames;
	UInt16				stringAreaOffset;
	UInt16				platformID;
	UInt16				platformSpecificID;
	UInt16				languageID;
	UInt16				nameID;
	UInt16				length;
	UInt16				offset;
} TTFontNamingTable;

