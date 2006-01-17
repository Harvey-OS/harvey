#ifndef ICC_H
#define ICC_H
/* 
 * International Color Consortium Format Library (icclib)
 *
 * Author:  Graeme W. Gill
 * Date:    99/11/29
 * Version: 2.01
 *
 * Copyright 1997, 1998, 1999, 2000, 2001 Graeme W. Gill
 * Please refer to Licence.txt file for details.
 */


/* We can get some subtle errors if certain headers aren't included */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <math.h>
#include <time.h>
#include <sys/types.h>

/*
 *  Note XYZ scaling to 1.0, not 100.0
 */


/* Make allowance for shared library use */
#ifdef ICCLIB_SHARED		/* Compiling or Using shared library version */
# ifdef ICCLIB_EXPORTS		/* Compiling shared library */
#  ifdef NT
#   define ICCLIB_API __declspec(dllexport)
#  endif /* NT */
# else						/* Using shared library */
#  ifdef NT
#   define ICCLIB_API __declspec(dllimport)
#   ifdef ICCLIB_DEBUG
#    pragma comment (lib, "icclibd.lib")
#   else
#    pragma comment (lib, "icclib.lib")
#   endif	/* DEBUG */
#  endif /* NT */
# endif
#else						/* Using static library */
# define ICCLIB_API	/* empty */
#endif


#ifdef __cplusplus
	extern "C" {
#endif


/* ---------------------------------------------- */
/* Platform specific defines */
/* It is assumed that the native machine size is 32 bits */
#ifndef INR8
#define INR8   signed char		/* 8 bit signed */
#endif
#ifndef INR16
#define INR16  signed short		/* 16 bit signed */
#endif
#ifndef INR32
#define INR32  signed long		/* 32 bit signed */
#endif
#ifndef ORD8
#define ORD8   unsigned char	/* 8 bit unsigned */
#endif
#ifndef ORD16
#define ORD16  unsigned short	/* 16 bit unsigned */
#endif
#ifndef ORD32
#define ORD32  unsigned long	/* 32 bit unsigned */
#endif

#include "icc9809.h"	/* Standard ICC definitions, version ICC.1:1998-09 with mods noted. */ 

/* Note that the prefix icm is used for the native Machine */
/* equivalents of the file structures defined in icc34.h */

/* ---------------------------------------------- */
/* System interface objects. The defaults can be replaced */
/* for adaption to different system environments */

/* File access class interface definition */
#define ICM_FILE_BASE																		\
	/* Public: */																			\
																							\
	/* Set current position to offset. Return 0 on success, nz on failure. */				\
	int    (*seek) (struct _icmFile *p, long int offset);									\
																							\
	/* Read count items of size length. Return number of items successfully read. */ 		\
	size_t (*read) (struct _icmFile *p, void *buffer, size_t size, size_t count);			\
																							\
	/* write count items of size length. Return number of items successfully written. */ 	\
	size_t (*write)(struct _icmFile *p, void *buffer, size_t size, size_t count);			\
																							\
	/* flush all write data out to secondary storage. Return nz on failure. */				\
	int (*flush)(struct _icmFile *p);														\
																							\
	/* we're done with the file object, return nz on failure */								\
	int (*del)(struct _icmFile *p);															\


/* Common file interface class */
struct _icmFile {
	ICM_FILE_BASE
}; typedef struct _icmFile icmFile;


/* - - - - - - - - - - - - - - - - - - - - -  */

/* Implementation of file access class based on standard file I/O */
struct _icmFileStd {
	ICM_FILE_BASE

	/* Private: */
	FILE *fp;
	int   doclose;		/* nz if free should close */
}; typedef struct _icmFileStd icmFileStd;

/* Create given a file name */
icmFile *new_icmFileStd_name(char *name, char *mode);

/* Create given a (binary) FILE* */
icmFile *new_icmFileStd_fp(FILE *fp);



/* - - - - - - - - - - - - - - - - - - - - -  */
/* Implementation of file access class based on a memory image */
struct _icmFileMem {
	ICM_FILE_BASE

	/* Private: */
	unsigned char *start, *cur, *end;

}; typedef struct _icmFileMem icmFileMem;

/* Create a memory image file access class */
icmFile *new_icmFileMem(void *base, size_t length);


/* - - - - - - - - - - - - - - - - - - - - -  */
/* Heap allocator class interface definition */
#define ICM_ALLOC_BASE																		\
	/* Public: */																			\
																							\
	void *(*malloc) (struct _icmAlloc *p, size_t size);										\
	void *(*calloc) (struct _icmAlloc *p, size_t num, size_t size);							\
	void *(*realloc)(struct _icmAlloc *p, void *ptr, size_t size);							\
	void  (*free)   (struct _icmAlloc *p, void *ptr);										\
																							\
	/* we're done with the allocator object */												\
	void (*del)(struct _icmAlloc *p);														\

/* Common heap allocator interface class */
struct _icmAlloc {
	ICM_ALLOC_BASE
}; typedef struct _icmAlloc icmAlloc;

/* - - - - - - - - - - - - - - - - - - - - -  */

/* Implementation of heap class based on standard system malloc */
struct _icmAllocStd {
	ICM_ALLOC_BASE
}; typedef struct _icmAllocStd icmAllocStd;

/* Create a standard alloc object */
icmAlloc *new_icmAllocStd(void);


/* --------------------------------- */
/* Assumed constants                 */

#define MAX_CHAN 15		/* Maximum number of color channels */

/* --------------------------------- */
/* tag and other compound structures */

typedef int icmSig;	/* Otherwise un-enumerated 4 byte signature */

typedef struct {
	ORD32 l;			/* High and low components of signed 64 bit */
	INR32 h;
} icmInt64;

typedef struct {
	ORD32 l,h;			/* High and low components of unsigned 64 bit */
} icmUint64;

/* XYZ Number */
typedef struct {
    double  X;
    double  Y;
    double  Z;
} icmXYZNumber;

/* Response 16 number */
typedef struct {
	double	       deviceValue;	/* The device value in range 0.0 - 1.0 */
	double	       measurement;	/* The reading value */
} icmResponse16Number;

/*
 *  read and write method error codes:
 *  0 = sucess
 *  1 = file format/logistical error
 *  2 = system error
 */

#define ICM_BASE_MEMBERS																\
	/* Private: */																		\
	icTagTypeSignature  ttype;		/* The tag type signature */						\
	struct _icc    *icp;			/* Pointer to ICC we're a part of */				\
	int	           touched;			/* Flag for write bookeeping */						\
    int            refcount;		/* Reference count for sharing */					\
	unsigned int   (*get_size)(struct _icmBase *p);										\
	int            (*read)(struct _icmBase *p, unsigned long len, unsigned long of);	\
	int            (*write)(struct _icmBase *p, unsigned long of);						\
	void           (*del)(struct _icmBase *p);											\
																						\
	/* Public: */																		\
	void           (*dump)(struct _icmBase *p, FILE *op, int verb);						\
	int            (*allocate)(struct _icmBase *p);									

/* Base tag element data object */
struct _icmBase {
	ICM_BASE_MEMBERS
}; typedef struct _icmBase icmBase;

/* UInt8 Array */
struct _icmUInt8Array {
	ICM_BASE_MEMBERS

	/* Private: */
	unsigned int   _size;		/* Size currently allocated */

	/* Public: */
	unsigned long	size;		/* Allocated and used size of the array */
    unsigned int   *data;		/* Pointer to array of data */ 
}; typedef struct _icmUInt8Array icmUInt8Array;

/* uInt16 Array */
struct _icmUInt16Array {
	ICM_BASE_MEMBERS

	/* Private: */
	unsigned int   _size;		/* Size currently allocated */

	/* Public: */
	unsigned long	size;		/* Allocated and used size of the array */
    unsigned int	*data;		/* Pointer to array of data */ 
}; typedef struct _icmUInt16Array icmUInt16Array;

/* uInt32 Array */
struct _icmUInt32Array {
	ICM_BASE_MEMBERS

	/* Private: */
	unsigned int   _size;		/* Size currently allocated */

	/* Public: */
	unsigned long	size;		/* Allocated and used size of the array */
    unsigned int	*data;		/* Pointer to array of data */ 
}; typedef struct _icmUInt32Array icmUInt32Array;

/* UInt64 Array */
struct _icmUInt64Array {
	ICM_BASE_MEMBERS

	/* Private: */
	unsigned int   _size;		/* Size currently allocated */

	/* Public: */
	unsigned long	size;		/* Allocated and used size of the array */
    icmUint64		*data;		/* Pointer to array of hight data */ 
}; typedef struct _icmUInt64Array icmUInt64Array;

/* u16Fixed16 Array */
struct _icmU16Fixed16Array {
	ICM_BASE_MEMBERS

	/* Private: */
	unsigned int   _size;		/* Size currently allocated */

	/* Public: */
	unsigned long	size;		/* Allocated and used size of the array */
    double			*data;		/* Pointer to array of hight data */ 
}; typedef struct _icmU16Fixed16Array icmU16Fixed16Array;

/* s15Fixed16 Array */
struct _icmS15Fixed16Array {
	ICM_BASE_MEMBERS

	/* Private: */
	unsigned int   _size;		/* Size currently allocated */

	/* Public: */
	unsigned long	size;		/* Allocated and used size of the array */
    double			*data;		/* Pointer to array of hight data */ 
}; typedef struct _icmS15Fixed16Array icmS15Fixed16Array;

/* XYZ Array */
struct _icmXYZArray {
	ICM_BASE_MEMBERS

	/* Private: */
	unsigned int   _size;		/* Size currently allocated */

	/* Public: */
	unsigned long	size;		/* Allocated and used size of the array */
    icmXYZNumber	*data;		/* Pointer to array of data */ 
}; typedef struct _icmXYZArray icmXYZArray;

/* Curve */
typedef enum {
    icmCurveUndef           = -1, /* Undefined curve */
    icmCurveLin             = 0,  /* Linear transfer curve */
    icmCurveGamma           = 1,  /* Gamma power transfer curve */
    icmCurveSpec            = 2   /* Specified curve */
} icmCurveStyle;

/* Curve reverse lookup information */
typedef struct {
	int inited;				/* Flag */
	double rmin, rmax;		/* Range of reverse grid */
	double qscale;			/* Quantising scale factor */
	long rsize;				/* Number of reverse lists */
	int **rlists;			/* Array of list of fwd values that may contain output value */
							/* Offset 0 = allocated size */
							/* Offset 1 = next free index */
							/* Offset 2 = first fwd index */
	unsigned long size;		/* Copy of forward table size */
	double       *data;		/* Copy of forward table data */
} icmRevTable;

struct _icmCurve {
	ICM_BASE_MEMBERS

	/* Private: */
	unsigned int   _size;		/* Size currently allocated */
	icmRevTable  rt;			/* Reverse table information */

	/* Public: */
    icmCurveStyle   flag;		/* Style of curve */
	unsigned long	size;		/* Allocated and used size of the array */
    double         *data;  		/* Curve data scaled to range 0.0 - 1.0 */
								/* or data[0] = gamma value */
	/* Translate a value through the curve, return warning flags */
	int (*lookup_fwd) (struct _icmCurve *p, double *out, double *in);	/* Forwards */
	int (*lookup_bwd) (struct _icmCurve *p, double *out, double *in);	/* Backwards */

}; typedef struct _icmCurve icmCurve;

/* Data */
typedef enum {
    icmDataUndef           = -1, /* Undefined data curve */
    icmDataASCII           = 0,  /* ASCII data curve */
    icmDataBin             = 1   /* Binary data */
} icmDataStyle;

struct _icmData {
	ICM_BASE_MEMBERS

	/* Private: */
	unsigned int   _size;		/* Size currently allocated */

	/* Public: */
    icmDataStyle	flag;		/* Style of data */
	unsigned long	size;		/* Allocated and used size of the array (inc ascii null) */
    unsigned char	*data;  	/* data or string, NULL if size == 0 */
}; typedef struct _icmData icmData;

/* text */
struct _icmText {
	ICM_BASE_MEMBERS

	/* Private: */
	unsigned int   _size;		/* Size currently allocated */

	/* Public: */
	unsigned long	 size;		/* Allocated and used size of desc, inc null */
	char             *data;		/* ascii string (null terminated), NULL if size==0 */
}; typedef struct _icmText icmText;

/* The base date time number */
struct _icmDateTimeNumber {
	ICM_BASE_MEMBERS

	/* Public: */
    unsigned int      year;
    unsigned int      month;
    unsigned int      day;
    unsigned int      hours;
    unsigned int      minutes;
    unsigned int      seconds;
}; typedef struct _icmDateTimeNumber icmDateTimeNumber;

#ifdef NEW
/ * DeviceSettings */

/*
   I think this all works like this:

Valid setting = (   (platform == platform1 and platform1.valid)
                 or (platform == platform2 and platform2.valid)
                 or ...
                )

where
	platformN.valid = (   platformN.combination1.valid
	                   or platformN.combination2.valid
	                   or ...
	                  )

where
	platformN.combinationM.valid = (    platformN.combinationM.settingstruct1.valid
	                                and platformN.combinationM.settingstruct2.valid
	                                and ...
	                               )

where
	platformN.combinationM.settingstructP.valid = (   platformN.combinationM.settingstructP.setting1.valid
	                                               or platformN.combinationM.settingstructP.setting2.valid
	                                               or ...
	                                              )

 */

/* The Settings Structure holds an array of settings of a particular type */
struct _icmSettingStruct {
	ICM_BASE_MEMBERS

	/* Private: */
	unsigned int   _num;				/* Size currently allocated */

	/* Public: */
	icSettingsSig       settingSig;		/* Setting identification */
	unsigned long       numSettings; 	/* number of setting values */
	union {								/* Setting values - type depends on Sig */
		icUInt64Number      *resolution;
		icDeviceMedia       *media;
		icDeviceDither      *halftone;
	}
}; typedef struct _icmSettingStruct icmSettingStruct;

/* A Setting Combination holds all arrays of different setting types */
struct _icmSettingComb {
	/* Private: */
	unsigned int   _num;			/* number currently allocated */

	/* Public: */
	unsigned long       numStructs;   /* num of setting structures */
	icmSettingStruct    *data;
}; typedef struct _icmSettingComb icmSettingComb;

/* A Platform Entry holds all setting combinations */
struct _icmPlatformEntry {
	/* Private: */
	unsigned int   _num;			/* number currently allocated */

	/* Public: */
	icPlatformSignature platform;
	unsigned long       numCombinations;    /* num of settings and allocated array size */
	icmSettingComb      *data; 
}; typedef struct _icmPlatformEntry icmPlatformEntry;

/* The Device Settings holds all platform settings */
struct _icmDeviceSettings {
	/* Private: */
	unsigned int   _num;			/* number currently allocated */

	/* Public: */
	unsigned long       numPlatforms;	/* num of platforms and allocated array size */
	icmPlatformEntry    *data;			/* Array of pointers to platform entry data */
}; typedef struct _icmDeviceSettings icmDeviceSettings;

#endif /* NEW */

/* lut */
struct _icmLut {
	ICM_BASE_MEMBERS

	/* Private: */
	/* Cache appropriate normalization routines */
	int dinc[MAX_CHAN];				/* Dimensional increment through clut */
	int dcube[1 << MAX_CHAN];		/* Hyper cube offsets */
	icmRevTable  rit;				/* Reverse input table information */
	icmRevTable  rot;				/* Reverse output table information */

	unsigned int inputTable_size;	/* size allocated to input table */
	unsigned int clutTable_size;	/* size allocated to clut table */
	unsigned int outputTable_size;	/* size allocated to output table */

	/* return the minimum and maximum values of the given channel in the clut */
	void (*min_max) (struct _icmLut *pp, double *minv, double *maxv, int chan);

	/* Translate color values through 3x3 matrix, input tables only, multi-dimensional lut, */
	/* or output tables, */
	int (*lookup_matrix)  (struct _icmLut *pp, double *out, double *in);
	int (*lookup_input)   (struct _icmLut *pp, double *out, double *in);
	int (*lookup_clut_nl) (struct _icmLut *pp, double *out, double *in);
	int (*lookup_clut_sx) (struct _icmLut *pp, double *out, double *in);
	int (*lookup_output)  (struct _icmLut *pp, double *out, double *in);

	/* Public: */

	/* return non zero if matrix is non-unity */
	int (*nu_matrix) (struct _icmLut *pp);

    unsigned int	inputChan;      /* Num of input channels */
    unsigned int	outputChan;     /* Num of output channels */
    unsigned int	clutPoints;     /* Num of grid points */
    unsigned int	inputEnt;       /* Num of in-table entries (must be 256 for Lut8) */
    unsigned int	outputEnt;      /* Num of out-table entries (must be 256 for Lut8) */
    double			e[3][3];		/* 3 * 3 array */
	double	        *inputTable;	/* The in-table: [inputChan * inputEnt] */
	double	        *clutTable;		/* The clut: [(clutPoints ^ inputChan) * outputChan] */
	double	        *outputTable;	/* The out-table: [outputChan * outputEnt] */
	/* inputTable  is organized [inputChan 0..ic-1][inputEnt 0..ie-1] */
	/* clutTable   is organized [inputChan 0, 0..cp-1]..[inputChan ic-1, 0..cp-1]
	                                                                [outputChan 0..oc-1] */
	/* outputTable is organized [outputChan 0..oc-1][outputEnt 0..oe-1] */

	/* Helper function to setup the three tables contents */
	int (*set_tables) (
		struct _icmLut *p,						/* Pointer to Lut object */
		void   *cbctx,							/* Opaque callback context pointer value */
		icColorSpaceSignature insig, 			/* Input color space */
		icColorSpaceSignature outsig, 			/* Output color space */
		void (*infunc)(void *cbctx, double *out, double *in),
								/* Input transfer function, inspace->inspace' (NULL = default) */
		double *inmin, double *inmax,			/* Maximum range of inspace' values */
												/* (NULL = default) */
		void (*clutfunc)(void *cbntx, double *out, double *in),
								/* inspace' -> outspace' transfer function */
		double *clutmin, double *clutmax,		/* Maximum range of outspace' values */
												/* (NULL = default) */
		void (*outfunc)(void *cbntx, double *out, double *in));
								/* Output transfer function, outspace'->outspace (NULL = deflt) */
		
}; typedef struct _icmLut icmLut;

/* Measurement Data */
struct _icmMeasurement {
	ICM_BASE_MEMBERS

	/* Public: */
    icStandardObserver           observer;       /* Standard observer */
    icmXYZNumber                 backing;        /* XYZ for backing */
    icMeasurementGeometry        geometry;       /* Meas. geometry */
    double                       flare;          /* Measurement flare */
    icIlluminant                 illuminant;     /* Illuminant */
}; typedef struct _icmMeasurement icmMeasurement;

/* Named color */

/* Structure that holds each named color data */
typedef struct {
	struct _icc      *icp;				/* Pointer to ICC we're a part of */
	char              root[32];			/* Root name for color */
	double            pcsCoords[3];		/* icmNC2: PCS coords of color */
	double            deviceCoords[MAX_CHAN];	/* Dev coords of color */
} icmNamedColorVal;

struct _icmNamedColor {
	ICM_BASE_MEMBERS

	/* Private: */
	unsigned int      _count;			/* Count currently allocated */

	/* Public: */
    unsigned int      vendorFlag;		/* Bottom 16 bits for IC use */
    unsigned int      count;			/* Count of named colors */
    unsigned int      nDeviceCoords;	/* Num of device coordinates */
    char              prefix[32];		/* Prefix for each color name (null terminated) */
    char              suffix[32];		/* Suffix for each color name (null terminated) */
    icmNamedColorVal  *data;			/* Array of [count] color values */
}; typedef struct _icmNamedColor icmNamedColor;

/* textDescription */
struct _icmTextDescription {
	ICM_BASE_MEMBERS

	/* Private: */
	unsigned long  _size;			/* Size currently allocated */
	unsigned long  uc_size;			/* uc Size currently allocated */
	int            (*core_read)(struct _icmTextDescription *p, char **bpp, char *end);
	int            (*core_write)(struct _icmTextDescription *p, char **bpp);

	/* Public: */
	unsigned long	  size;			/* Allocated and used size of desc, inc null */
	char              *desc;		/* ascii string (null terminated) */

	unsigned int      ucLangCode;	/* UniCode language code */
	unsigned long	  ucSize;		/* Allocated and used size of ucDesc in wchars, inc null */
	ORD16             *ucDesc;		/* The UniCode description (null terminated) */

	ORD16             scCode;		/* ScriptCode code */
	unsigned long	  scSize;		/* Used size of scDesc in bytes, inc null */
	ORD8              scDesc[67];	/* ScriptCode Description (null terminated, max 67) */
}; typedef struct _icmTextDescription icmTextDescription;

/* Profile sequence structure */
struct _icmDescStruct {
	/* Private: */
	struct _icc      *icp;				/* Pointer to ICC we're a part of */

	/* Public: */
	int             (*allocate)(struct _icmDescStruct *p);	/* Allocate method */
    icmSig            deviceMfg;		/* Dev Manufacturer */
    unsigned int      deviceModel;		/* Dev Model */
    icmUint64         attributes;		/* Dev attributes */
    icTechnologySignature technology;	/* Technology sig */
	icmTextDescription device;			/* Manufacturer text (sub structure) */
	icmTextDescription model;			/* Model text (sub structure) */
}; typedef struct _icmDescStruct icmDescStruct;

/* Profile sequence description */
struct _icmProfileSequenceDesc {
	ICM_BASE_MEMBERS

	/* Private: */
	unsigned int	 _count;			/* number currently allocated */

	/* Public: */
    unsigned int      count;			/* Number of descriptions */
	icmDescStruct     *data;			/* array of [count] descriptions */
}; typedef struct _icmProfileSequenceDesc icmProfileSequenceDesc;

/* signature (only ever used for technology ??) */
struct _icmSignature {
	ICM_BASE_MEMBERS

	/* Public: */
    icTechnologySignature sig;	/* Signature */
}; typedef struct _icmSignature icmSignature;

/* Per channel Screening Data */
typedef struct {
	/* Public: */
    double            frequency;		/* Frequency */
    double            angle;			/* Screen angle */
    icSpotShape       spotShape;		/* Spot Shape encodings below */
} icmScreeningData;

struct _icmScreening {
	ICM_BASE_MEMBERS

	/* Private: */
	unsigned int   _channels;			/* number currently allocated */

	/* Public: */
    unsigned int      screeningFlag;	/* Screening flag */
    unsigned int      channels;			/* Number of channels */
    icmScreeningData  *data;			/* Array of screening data */
}; typedef struct _icmScreening icmScreening;

/* Under color removal, black generation */
struct _icmUcrBg {
	ICM_BASE_MEMBERS

	/* Private: */
    unsigned int      UCR_count;		/* Currently allocated UCR count */
    unsigned int      BG_count;			/* Currently allocated BG count */
	unsigned long	  _size;			/* Currently allocated string size */

	/* Public: */
    unsigned int      UCRcount;			/* Undercolor Removal Curve length */
    double           *UCRcurve;		    /* The array of UCR curve values, 0.0 - 1.0 */
										/* or 0.0 - 100 % if count = 1 */
    unsigned int      BGcount;			/* Black generation Curve length */
    double           *BGcurve;			/* The array of BG curve values, 0.0 - 1.0 */
										/* or 0.0 - 100 % if count = 1 */
	unsigned long	  size;				/* Allocated and used size of desc, inc null */
	char              *string;			/* UcrBg description (null terminated) */
}; typedef struct _icmUcrBg icmUcrBg;

/* viewingConditionsType */
struct _icmViewingConditions {
	ICM_BASE_MEMBERS

	/* Public: */
    icmXYZNumber    illuminant;		/* In candelas per sq. meter */
    icmXYZNumber    surround;		/* In candelas per sq. meter */
    icIlluminant    stdIlluminant;	/* See icIlluminant defines */
}; typedef struct _icmViewingConditions icmViewingConditions;

/* Postscript Color Rendering Dictionary names type */
struct _icmCrdInfo {
	ICM_BASE_MEMBERS
	/* Private: */
    unsigned long    _ppsize;		/* Currently allocated size */
	unsigned long    _crdsize[4];	/* Currently allocated sizes */

	/* Public: */
    unsigned long    ppsize;		/* Postscript product name size (including null) */
    char            *ppname;		/* Postscript product name (null terminated) */
	unsigned long    crdsize[4];	/* Rendering intent 0-3 CRD names sizes (icluding null) */
	char            *crdname[4];	/* Rendering intent 0-3 CRD names (null terminated) */
}; typedef struct _icmCrdInfo icmCrdInfo;


/* Apple ColorSync 2.5 video card gamma type */
struct _icmVideoCardGammaTable {
	unsigned short   channels;		/* # of gamma channels (1 or 3) */
	unsigned short   entryCount; 	/* 1-based number of entries per channel */
	unsigned short   entrySize;		/* size in bytes of each entry */
	void            *data;			/* variable size data */
}; typedef struct _icmVideoCardGammaTable icmVideoCardGammaTable;

struct _icmVideoCardGammaFormula {
	double           redGamma;		/* must be > 0.0 */
	double           redMin;		/* must be > 0.0 and < 1.0 */
	double           redMax;		/* must be > 0.0 and < 1.0 */
	double           greenGamma;   	/* must be > 0.0 */
	double           greenMin;		/* must be > 0.0 and < 1.0 */
	double           greenMax;		/* must be > 0.0 and < 1.0 */
	double           blueGamma;		/* must be > 0.0 */
	double           blueMin;		/* must be > 0.0 and < 1.0 */
	double           blueMax;		/* must be > 0.0 and < 1.0 */
}; typedef struct _icmVideoCardGammaFormula icmVideoCardGammaFormula;

enum {
	icmVideoCardGammaTableType = 0,
	icmVideoCardGammaFormulaType = 1
};

struct _icmVideoCardGamma {
	ICM_BASE_MEMBERS
	unsigned long                tagType;		/* eg. table or formula, use above enum */
	union {
		icmVideoCardGammaTable   table;
		icmVideoCardGammaFormula formula;
	} u;
}; typedef struct _icmVideoCardGamma icmVideoCardGamma;

/* ------------------------------------------------- */
/* The Profile header */
struct _icmHeader {
	/* Private: */
	unsigned int           (*get_size)(struct _icmHeader *p);
	int                    (*read)(struct _icmHeader *p, unsigned long len, unsigned long of);
	int                    (*write)(struct _icmHeader *p, unsigned long of);
	void                   (*del)(struct _icmHeader *p);
	struct _icc            *icp;			/* Pointer to ICC we're a part of */
    unsigned int            size;			/* Profile size in bytes */

	/* public: */
	void                   (*dump)(struct _icmHeader *p, FILE *op, int verb);

	/* Values that must be set before writing */
    icProfileClassSignature deviceClass;	/* Type of profile */
    icColorSpaceSignature   colorSpace;		/* Clr space of data */
    icColorSpaceSignature   pcs;			/* PCS: XYZ or Lab */
    icRenderingIntent       renderingIntent;/* Rendering intent */

	/* Values that should be set before writing */
    icmSig                  manufacturer;	/* Dev manufacturer */
    icmSig		            model;			/* Dev model */
    icmUint64               attributes;		/* Device attributes.l */
    unsigned int            flags;			/* Various bits */

	/* Values that may optionally be set before writing */
    /* icmUint64            attributes;		   Device attributes.h (see above) */
    icmSig                  creator;		/* Profile creator */

	/* Values that are not normally set, since they have defaults */
    icmSig                  cmmId;			/* CMM for profile */
    int            			majv, minv, bfv;/* Format version - major, minor, bug fix */
    icmDateTimeNumber       date;			/* Creation Date */
    icPlatformSignature     platform;		/* Primary Platform */
    icmXYZNumber            illuminant;		/* Profile illuminant */

}; typedef struct _icmHeader icmHeader;

/* ---------------------------------------------------------- */
/* Objects for accessing lookup functions */

/* Public: Parameter to get_luobj function */
typedef enum {
    icmFwd           = 0,  /* Device to PCS, or Device 1 to Last Device */
    icmBwd           = 1,  /* PCS to Device, or Last Device to Device */
    icmGamut         = 2,  /* PCS Gamut check */
    icmPreview       = 3   /* PCS to PCS preview */
} icmLookupFunc;

/* Public: Parameter to get_luobj function */
typedef enum {
    icmLuOrdNorm     = 0,  /* Normal profile preference: Lut, matrix, monochrome */
    icmLuOrdRev      = 1   /* Reverse profile preference: monochrome, matrix, monochrome */
} icmLookupOrder;

/* Public: Lookup algorithm object type */
typedef enum {
    icmMonoFwdType       = 0,	/* Monochrome, Forward */
    icmMonoBwdType       = 1,	/* Monochrome, Backward */
    icmMatrixFwdType     = 2,	/* Matrix, Forward */
    icmMatrixBwdType     = 3,	/* Matrix, Backward */
    icmLutType           = 4	/* Multi-dimensional Lookup Table */
} icmLuAlgType;

#define LU_ICM_BASE_MEMBERS																	\
	/* Private: */																		\
	icmLuAlgType   ttype;		    	/* The object tag */							\
	struct _icc    *icp;				/* Pointer to ICC we're a part of */			\
	icRenderingIntent intent;			/* Effective intent */							\
	icmLookupFunc function;				/* Functionality being used */					\
	icmXYZNumber pcswht, whitePoint, blackPoint;	/* White and black point info */	\
	double toAbs[3][3];					/* Matrix to convert from relative to absolute */ \
	double fromAbs[3][3];				/* Matrix to convert from absolute to relative */ \
    icColorSpaceSignature inSpace;		/* Native Clr space of input */					\
    icColorSpaceSignature outSpace;		/* Native Clr space of output */				\
	icColorSpaceSignature pcs;			/* Native PCS */								\
    icColorSpaceSignature e_inSpace;	/* Effective Clr space of input */				\
    icColorSpaceSignature e_outSpace;	/* Effective Clr space of output */				\
	icColorSpaceSignature e_pcs;		/* Effective PCS */								\
																						\
	/* Public: */																		\
	void           (*del)(struct _icmLuBase *p);										\
	                            /* Internal native colorspaces */     							\
	void           (*lutspaces) (struct _icmLuBase *p, icColorSpaceSignature *ins, int *inn,	\
	                                                   icColorSpaceSignature *outs, int *outn);	\
	                                                                                        \
	                         /* External effecive colorspaces */							\
	void           (*spaces) (struct _icmLuBase *p, icColorSpaceSignature *ins, int *inn,	\
	                                 icColorSpaceSignature *outs, int *outn,				\
	                                 icmLuAlgType *alg, icRenderingIntent *intt, 			\
	                                 icmLookupFunc *fnc, icColorSpaceSignature *pcs); 		\
																							\
	/* Get the effective input space and output space ranges */								\
	void (*get_ranges) (struct _icmLuBase *p,												\
		double *inmin, double *inmax,		/* Maximum range of inspace values */			\
		double *outmin, double *outmax);	/* Maximum range of outspace values */			\
																							\
	void           (*wh_bk_points)(struct _icmLuBase *p, icmXYZNumber *wht, icmXYZNumber *blk);	\
	int            (*lookup) (struct _icmLuBase *p, double *out, double *in);

	/* Translate color values through profile */
	/* 0 = success */
	/* 1 = warning: clipping occured */
	/* 2 = fatal: other error */

/* Base lookup object */
struct _icmLuBase {
	LU_ICM_BASE_MEMBERS
}; typedef struct _icmLuBase icmLuBase;

/* Monochrome  Fwd & Bwd type object */
struct _icmLuMono {
	LU_ICM_BASE_MEMBERS
	icmCurve    *grayCurve;

	/* Overall lookups */
	int (*fwd_lookup) (struct _icmLuBase *p, double *out, double *in);
	int (*bwd_lookup) (struct _icmLuBase *p, double *out, double *in);

	/* Components of lookup */
	int (*fwd_curve) (struct _icmLuMono *p, double *out, double *in);
	int (*fwd_map)   (struct _icmLuMono *p, double *out, double *in);
	int (*fwd_abs)   (struct _icmLuMono *p, double *out, double *in);
	int (*bwd_abs)   (struct _icmLuMono *p, double *out, double *in);
	int (*bwd_map)   (struct _icmLuMono *p, double *out, double *in);
	int (*bwd_curve) (struct _icmLuMono *p, double *out, double *in);

}; typedef struct _icmLuMono icmLuMono;

/* 3D Matrix Fwd & Bwd type object */
struct _icmLuMatrix {
	LU_ICM_BASE_MEMBERS
	icmCurve    *redCurve, *greenCurve, *blueCurve;
	icmXYZArray *redColrnt, *greenColrnt, *blueColrnt;
    double		mx[3][3];	/* 3 * 3 conversion matrix */
    double		bmx[3][3];	/* 3 * 3 backwards conversion matrix */

	/* Overall lookups */
	int (*fwd_lookup) (struct _icmLuBase *p, double *out, double *in);
	int (*bwd_lookup) (struct _icmLuBase *p, double *out, double *in);

	/* Components of lookup */
	int (*fwd_curve)  (struct _icmLuMatrix *p, double *out, double *in);
	int (*fwd_matrix) (struct _icmLuMatrix *p, double *out, double *in);
	int (*fwd_abs)    (struct _icmLuMatrix *p, double *out, double *in);
	int (*bwd_abs)    (struct _icmLuMatrix *p, double *out, double *in);
	int (*bwd_matrix) (struct _icmLuMatrix *p, double *out, double *in);
	int (*bwd_curve)  (struct _icmLuMatrix *p, double *out, double *in);

}; typedef struct _icmLuMatrix icmLuMatrix;

/* Multi-D. Lut type object */
struct _icmLuLut {
	LU_ICM_BASE_MEMBERS

	/* private: */
	icmLut *lut;								/* Lut to use */
	int    usematrix;							/* non-zero if matrix should be used */
    double imx[3][3];							/* 3 * 3 inverse conversion matrix */
	int    imx_valid;							/* Inverse matrix is valid */
	void (*in_normf)(double *out, double *in);	/* Lut input data normalizing function */
	void (*in_denormf)(double *out, double *in);/* Lut input data de-normalizing function */
	void (*out_normf)(double *out, double *in);	/* Lut output data normalizing function */
	void (*out_denormf)(double *out, double *in);/* Lut output de-normalizing function */
	void (*e_in_denormf)(double *out, double *in);/* Effective input de-normalizing function */
	void (*e_out_denormf)(double *out, double *in);/* Effecive output de-normalizing function */
	/* function chosen out of lut->lookup_clut_sx and lut->lookup_clut_nl to imp. clut() */
	int (*lookup_clut) (struct _icmLut *pp, double *out, double *in);	/* clut function */

	/* public: */

	/* Components of lookup */
	int (*in_abs)  (struct _icmLuLut *p, double *out, double *in);	/* Should be in icmLut ? */
	int (*matrix)  (struct _icmLuLut *p, double *out, double *in);
	int (*input)   (struct _icmLuLut *p, double *out, double *in);
	int (*clut)    (struct _icmLuLut *p, double *out, double *in);
	int (*output)  (struct _icmLuLut *p, double *out, double *in);
	int (*out_abs) (struct _icmLuLut *p, double *out, double *in);	/* Should be in icmLut ? */

	/* Some inverse components */
	/* Should be in icmLut ??? */
	int (*inv_out_abs) (struct _icmLuLut *p, double *out, double *in);
	int (*inv_output)  (struct _icmLuLut *p, double *out, double *in);
	/* inv_clut is beyond scope of icclib. See argyll for solution! */
	int (*inv_input)   (struct _icmLuLut *p, double *out, double *in);
	int (*inv_matrix)  (struct _icmLuLut *p, double *out, double *in);
	int (*inv_in_abs)  (struct _icmLuLut *p, double *out, double *in);

	/* Get various types of information about the LuLut */
	void (*get_info) (struct _icmLuLut *p, icmLut **lutp,
	                 icmXYZNumber *pcswhtp, icmXYZNumber *whitep,
	                 icmXYZNumber *blackp);

	/* Get the native input space and output space ranges */
	void (*get_lutranges) (struct _icmLuLut *p,
		double *inmin, double *inmax,		/* Maximum range of inspace values */
		double *outmin, double *outmax);	/* Maximum range of outspace values */

	/* Get the matrix contents */
	void (*get_matrix) (struct _icmLuLut *p, double m[3][3]);

}; typedef struct _icmLuLut icmLuLut;

/* ---------------------------------------------------------- */
/* A tag */
typedef struct {
    icTagSignature      sig;			/* The tag signature */
	icTagTypeSignature  ttype;			/* The tag type signature */
    unsigned int        offset;			/* File offset to start header */
    unsigned int        size;			/* Size in bytes */
	icmBase            *objp;			/* In memory data structure */
} icmTag;

/* Pseudo enumerations valid as parameter to get_luobj(): */

/* To be specified where an intent is not appropriate */
#define icmDefaultIntent ((icRenderingIntent)98)

/* Pseudo PCS colospace used to indicate the native PCS */
#define icmSigDefaultData ((icColorSpaceSignature) 0x0)


/* The ICC object */
struct _icc {
	/* Public: */
	unsigned int (*get_size)(struct _icc *p);				/* Return total size needed, 0 = err. */
	int          (*read)(struct _icc *p, icmFile *fp, unsigned long of);	/* Returns error code */
	int          (*write)(struct _icc *p, icmFile *fp, unsigned long of);/* Returns error code */
	void         (*dump)(struct _icc *p, FILE *op, int verb);	/* Dump whole icc */
	void         (*del)(struct _icc *p);						/* Free whole icc */
	int          (*find_tag)(struct _icc *p, icTagSignature sig);
															/* Returns 1 if found, 2 readable */
	icmBase *    (*read_tag)(struct _icc *p, icTagSignature sig);
															/* Returns pointer to object */
	icmBase *    (*add_tag)(struct _icc *p, icTagSignature sig, icTagTypeSignature ttype);
															/* Returns pointer to object */
	int          (*rename_tag)(struct _icc *p, icTagSignature sig, icTagSignature sigNew);
															/* Rename and existing tag */
	icmBase *    (*link_tag)(struct _icc *p, icTagSignature sig, icTagSignature ex_sig);
															/* Returns pointer to object */
	int          (*unread_tag)(struct _icc *p, icTagSignature sig);
														/* Unread a tag (free on refcount == 0 */
	int          (*read_all_tags)(struct _icc *p); /* Read all the tags, non-zero on error. */

	int          (*delete_tag)(struct _icc *p, icTagSignature sig);
															/* Returns 0 if deleted OK */
	icmLuBase *  (*get_luobj) (struct _icc *p,
                                     icmLookupFunc func,			/* Functionality */
	                                 icRenderingIntent intent,		/* Intent */
	                                 icColorSpaceSignature pcsor,	/* PCS overide (0 = def) */
	                                 icmLookupOrder order);			/* Search Order */
	                                 /* Return appropriate lookup object */
	                                 /* NULL on error, check errc+err for reason */

	
    icmHeader       *header;			/* The header */
	char             err[512];			/* Error message */
	int              errc;				/* Error code */

	/* Private: ? */
	icmAlloc        *al;				/* Heap allocator */
	int              del_al;			/* NZ if heap allocator should be deleted */
	icmFile         *fp;				/* File associated with object */
	unsigned long    of;				/* Offset of the profile within the file */
    unsigned int     count;				/* Num tags in the profile */
    icmTag          *data;    			/* The tagTable and tagData */

	}; typedef struct _icc icc;

/* ========================================================== */
/* Utility structures and declarations */

/* Structure to hold pseudo-hilbert counter info */
struct _psh {
	int      di;	/* Dimensionality */
	unsigned res;	/* Resolution per coordinate */
	unsigned bits;	/* Bits per coordinate */
	unsigned ix;	/* Current binary index */
	unsigned tmask;	/* Total 2^n count mask */
	unsigned count;	/* Usable count */
}; typedef struct _psh psh;

/* Type of encoding to be returned as a string */
typedef enum {
    icmScreenEncodings,
    icmDeviceAttributes,
	icmProfileHeaderFlags,
	icmAsciiOrBinaryData,
	icmTagSignature,
	icmTechnologySignature,
	icmTypeSignature,
	icmColorSpaceSignature,
	icmProfileClassSignaure,
	icmPlatformSignature,
	icmMeasurementFlare,
	icmMeasurementGeometry,
	icmRenderingIntent,
	icmSpotShape,
	icmStandardObserver,
	icmIlluminant,
	icmLuAlg
} icmEnumType;

/* ========================================================== */
/* Public function declarations */
/* Create an empty object. Return null on error */
extern ICCLIB_API icc *new_icc(void);				/* Default allocator */
extern ICCLIB_API icc *new_icc_a(icmAlloc *al);		/* With allocator class */

/* - - - - - - - - - - - - - */
/* Some useful utilities: */

/* Return a string that represents a tag */
extern ICCLIB_API char *tag2str(int tag);

/* Return a tag created from a string */
extern ICCLIB_API int str2tag(const char *str);

/* Return a string description of the given enumeration value */
extern ICCLIB_API const char *icm2str(icmEnumType etype, int enumval);

/* CIE XYZ to perceptual Lab */
extern ICCLIB_API void icmXYZ2Lab(icmXYZNumber *w, double *out, double *in);

/* Perceptual Lab to CIE XYZ */
extern ICCLIB_API void icmLab2XYZ(icmXYZNumber *w, double *out, double *in);

/* The standard D50 illuminant value */
extern ICCLIB_API icmXYZNumber icmD50;

/* The standard D65 illuminant value */
extern ICCLIB_API icmXYZNumber icmD65;

/* The default black value */
extern ICCLIB_API icmXYZNumber icmBlack;

/* Initialise a pseudo-hilbert grid counter, return total usable count. */
extern ICCLIB_API unsigned psh_init(psh *p, int di, unsigned res, int co[]);

/* Reset the counter */
extern ICCLIB_API void psh_reset(psh *p);

/* Increment pseudo-hilbert coordinates */
/* Return non-zero if count rolls over to 0 */
extern ICCLIB_API int psh_inc(psh *p, int co[]);

/* Chromatic Adaption transform utility */
/* Return a 3x3 chromatic adaption matrix */
void icmChromAdaptMatrix(
	int flags,				/* Flags as defined below */
	icmXYZNumber d_wp,		/* Destination white point */
	icmXYZNumber s_wp,		/* Source white point */
	double mat[3][3]		/* Destination matrix */
);

#define ICM_CAM_BRADFORD	0x0001	/* Use Bradford sharpened response space */
#define ICM_CAM_MULMATRIX	0x0002	/* Transform the given matrix */

/* Return the normal Delta E given two Lab values */
extern ICCLIB_API double icmLabDE(double *in1, double *in2);

/* Return the normal Delta E squared, given two Lab values */
extern ICCLIB_API double icmLabDEsq(double *in1, double *in2);

/* Return the CIE94 Delta E color difference measure for two Lab values */
extern ICCLIB_API double icmCIE94(double *in1, double *in2);

/* Return the CIE94 Delta E color difference measure squared, for two Lab values */
extern ICCLIB_API double icmCIE94sq(double *in1, double *in2);

/* Simple macro to transfer an array to an XYZ number */
#define icmAry2XYZ(xyz, ary) ((xyz).X = (ary)[0], (xyz).Y = (ary)[1], (xyz).Z = (ary)[2])

/* And the reverse */
#define icmXYZ2Ary(ary, xyz) ((ary)[0] = (xyz).X, (ary)[1] = (xyz).Y, (ary)[2] = (xyz).Z)

/* ---------------------------------------------------------- */

#ifdef __cplusplus
	}
#endif

#endif /* ICC_H */

