/*
**	Copyright (c) 1989
**	Truevision Inc.
**	All Rights Reserved
*/

/*
**	Attempt to accomodate machine dependencies
**	The parameter, MSDOS, is automatically defined by the Microsoft
**	compiler.
*/

typedef short		SINT16; 		/* 16-bit signed integer			*/
typedef ushort		UINT16;			/* 16-bit unsigned integer			*/
typedef long		SINT32; 		/* 32-bit signed integer			*/
typedef ulong		UINT32;			/* 32-bit unsigned integer			*/
typedef char		SINT8;			/* 8-bit signed integer 			*/
typedef uchar		UINT8;			/* 8-bit unsigned integer			*/

/*
** Define image types for GetPic and PutPic
*/

#define TGAColorMapped		1
#define TGATrueColor		2
#define TGABlackWhite		3
#define TGARLEColorMapped	9
#define TGARLETrueColor		10
#define TGARLEBlackWhite	11

/****************************************************************************
**
**	For more information about the original Truevision TGA(tm) file format,
**	or for additional information about the new extensions to the
**	Truevision TGA file, refer to the "Truevision TGA File Format
**	Specification Version 2.0" available from Truevision or your
**	Truevision dealer.
**
**  FILE STRUCTURE FOR THE ORIGINAL TRUEVISION TGA FILE				
**	  FIELD 1 :	NUMBER OF CHARACTERS IN ID FIELD (1 BYTES)	
**	  FIELD 2 :	COLOR MAP TYPE (1 BYTES)			
**	  FIELD 3 :	IMAGE TYPE CODE (1 BYTES)			
**					= 0	NO IMAGE DATA INCLUDED		
**					= 1	UNCOMPRESSED, COLOR-MAPPED IMAGE
**					= 2	UNCOMPRESSED, TRUE-COLOR IMAGE	
**					= 3	UNCOMPRESSED, BLACK AND WHITE IMAGE
**					= 9	RUN-LENGTH ENCODED COLOR-MAPPED IMAGE
**					= 10 RUN-LENGTH ENCODED TRUE-COLOR IMAGE
**					= 11 RUN-LENGTH ENCODED BLACK AND WHITE IMAGE
**	  FIELD 4 :	COLOR MAP SPECIFICATION	(5 BYTES)		
**				4.1 : COLOR MAP ORIGIN (2 BYTES)	
**				4.2 : COLOR MAP LENGTH (2 BYTES)	
**				4.3 : COLOR MAP ENTRY SIZE (2 BYTES)	
**	  FIELD 5 :	IMAGE SPECIFICATION (10 BYTES)			
**				5.1 : X-ORIGIN OF IMAGE (2 BYTES)	
**				5.2 : Y-ORIGIN OF IMAGE (2 BYTES)	
**				5.3 : WIDTH OF IMAGE (2 BYTES)		
**				5.4 : HEIGHT OF IMAGE (2 BYTES)		
**				5.5 : IMAGE PIXEL SIZE (1 BYTE)		
**				5.6 : IMAGE DESCRIPTOR BYTE (1 BYTE) 	
**	  FIELD 6 :	IMAGE ID FIELD (LENGTH SPECIFIED BY FIELD 1)	
**	  FIELD 7 :	COLOR MAP DATA (BIT WIDTH SPECIFIED BY FIELD 4.3 AND
**				NUMBER OF COLOR MAP ENTRIES SPECIFIED IN FIELD 4.2)
**	  FIELD 8 :	IMAGE DATA FIELD (WIDTH AND HEIGHT SPECIFIED IN
**				FIELD 5.3 AND 5.4)				
****************************************************************************/

typedef struct _devDir
{
	UINT16	tagValue;
	UINT32	tagOffset;
	UINT32	tagSize;
} DevDir;

typedef struct _TGAFile
{
	UINT8	idLength;		/* length of ID string */
	UINT8	mapType;		/* color map type */
	UINT8	imageType;		/* image type code */
	UINT16	mapOrigin;		/* starting index of map */
	UINT16	mapLength;		/* length of map */
	UINT8	mapWidth;		/* width of map in bits */
	UINT16	xOrigin;		/* x-origin of image */
	UINT16	yOrigin;		/* y-origin of image */
	UINT16	imageWidth;		/* width of image */
	UINT16	imageHeight;	/* height of image */
	UINT8	pixelDepth;		/* bits per pixel */
	UINT8	imageDesc;		/* image descriptor */
	char	idString[256];	/* image ID string */
	UINT16	devTags;		/* number of developer tags in directory */
	DevDir	*devDirs;		/* pointer to developer directory entries */
	UINT16	extSize;		/* extension area size */
	char	author[41];		/* name of the author of the image */
	char	authorCom[4][81];	/* author's comments */
	UINT16	month;			/* date-time stamp */
	UINT16	day;
	UINT16	year;
	UINT16	hour;
	UINT16	minute;
	UINT16	second;
	char	jobID[41];		/* job identifier */
	UINT16	jobHours;		/* job elapsed time */
	UINT16	jobMinutes;
	UINT16	jobSeconds;
	char	softID[41];		/* software identifier/program name */
	UINT16	versionNum;		/* software version designation */
	UINT8	versionLet;
	UINT32	keyColor;		/* key color value as A:R:G:B */
	UINT16	pixNumerator;	/* pixel aspect ratio */
	UINT16	pixDenominator;
	UINT16	gammaNumerator;	/* gamma correction factor */
	UINT16	gammaDenominator;
	UINT32	colorCorrectOffset;	/* offset to color correction table */
	UINT32	stampOffset;	/* offset to postage stamp data */
	UINT32	scanLineOffset;	/* offset to scan line table */
	UINT8	alphaAttribute;	/* alpha attribute description */
	UINT32	*scanLineTable;	/* address of scan line offset table */
	UINT8	stampWidth;		/* width of postage stamp */
	UINT8	stampHeight;	/* height of postage stamp */
	void	*postStamp;		/* address of postage stamp data */
	UINT16	*colorCorrectTable;
	UINT32	extAreaOffset;	/* extension area offset */
	UINT32	devDirOffset;	/* developer directory offset */
	char	signature[18];	/* signature string	*/
} TGAFile;
