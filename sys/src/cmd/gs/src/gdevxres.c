/* Copyright (C) 1999 Aladdin Enterprises.  All rights reserved.

   This file is part of Aladdin Ghostscript.

   Aladdin Ghostscript is distributed with NO WARRANTY OF ANY KIND.  No author
   or distributor accepts any responsibility for the consequences of using it,
   or for whether it serves any particular purpose or works at all, unless he
   or she says so in writing.  Refer to the Aladdin Ghostscript Free Public
   License (the "License") for full details.

   Every copy of Aladdin Ghostscript must include a copy of the License,
   normally in a plain ASCII text file named PUBLIC.  The License grants you
   the right to copy, modify and redistribute Aladdin Ghostscript, but only
   under certain conditions described in the License.  Among other things, the
   License requires that the copyright notice and this notice be preserved on
   all copies.
 */

/*$Id: gdevxres.c,v 1.1 2000/03/09 08:40:41 lpd Exp $ */
/* X Windows driver resource tables */
#include "std.h"	/* must precede any file that includes <sys/types.h> */
#include "x_.h"
#include "gstypes.h"
#include "gsmemory.h"
#include "gxdevice.h"
#include "gdevx.h"

/*
 * We segregate these tables into their own file because the definition of
 * the XtResource structure is botched -- it declares the strings as char *
 * rather than const char * -- and so compiling the statically initialized
 * tables with gcc -Wcast-qual produces dozens of bogus warnings.
 */

XtResource gdev_x_resources[] =
{

/* (String) casts are here to suppress warnings about discarding `const' */
#define RINIT(a,b,t,s,o,it,n)\
  {(String)(a), (String)(b), (String)t, sizeof(s),\
   XtOffsetOf(gx_device_X, o), (String)it, (n)}
#define rpix(a,b,o,n)\
  RINIT(a,b,XtRPixel,Pixel,o,XtRString,(XtPointer)(n))
#define rdim(a,b,o,n)\
  RINIT(a,b,XtRDimension,Dimension,o,XtRImmediate,(XtPointer)(n))
#define rstr(a,b,o,n)\
  RINIT(a,b,XtRString,String,o,XtRString,(char*)(n))
#define rint(a,b,o,n)\
  RINIT(a,b,XtRInt,int,o,XtRImmediate,(XtPointer)(n))
#define rbool(a,b,o,n)\
  RINIT(a,b,XtRBoolean,Boolean,o,XtRImmediate,(XtPointer)(n))
#define rfloat(a,b,o,n)\
  RINIT(a,b,XtRFloat,float,o,XtRString,(XtPointer)(n))

    rpix(XtNbackground, XtCBackground, background, "XtDefaultBackground"),
    rpix(XtNborderColor, XtCBorderColor, borderColor, "XtDefaultForeground"),
    rdim(XtNborderWidth, XtCBorderWidth, borderWidth, 1),
    rstr("dingbatFonts", "DingbatFonts", dingbatFonts,
	 "ZapfDingbats: -Adobe-ITC Zapf Dingbats-Medium-R-Normal--"),
    rpix(XtNforeground, XtCForeground, foreground, "XtDefaultForeground"),
    rstr(XtNgeometry, XtCGeometry, geometry, NULL),
    rbool("logExternalFonts", "LogExternalFonts", logXFonts, False),
    rint("maxGrayRamp", "MaxGrayRamp", maxGrayRamp, 128),
    rint("maxRGBRamp", "MaxRGBRamp", maxRGBRamp, 5),
    rstr("palette", "Palette", palette, "Color"),

    /*
     * I had to compress the whitespace out of the default string to
     * satisfy certain balky compilers.
     */
    rstr("regularFonts", "RegularFonts", regularFonts, "\
AvantGarde-Book:-Adobe-ITC Avant Garde Gothic-Book-R-Normal--\n\
AvantGarde-BookOblique:-Adobe-ITC Avant Garde Gothic-Book-O-Normal--\n\
AvantGarde-Demi:-Adobe-ITC Avant Garde Gothic-Demi-R-Normal--\n\
AvantGarde-DemiOblique:-Adobe-ITC Avant Garde Gothic-Demi-O-Normal--\n\
Bookman-Demi:-Adobe-ITC Bookman-Demi-R-Normal--\n\
Bookman-DemiItalic:-Adobe-ITC Bookman-Demi-I-Normal--\n\
Bookman-Light:-Adobe-ITC Bookman-Light-R-Normal--\n\
Bookman-LightItalic:-Adobe-ITC Bookman-Light-I-Normal--\n\
Courier:-Adobe-Courier-Medium-R-Normal--\n\
Courier-Bold:-Adobe-Courier-Bold-R-Normal--\n\
Courier-BoldOblique:-Adobe-Courier-Bold-O-Normal--\n\
Courier-Oblique:-Adobe-Courier-Medium-O-Normal--\n\
Helvetica:-Adobe-Helvetica-Medium-R-Normal--\n\
Helvetica-Bold:-Adobe-Helvetica-Bold-R-Normal--\n\
Helvetica-BoldOblique:-Adobe-Helvetica-Bold-O-Normal--\n\
Helvetica-Narrow:-Adobe-Helvetica-Medium-R-Narrow--\n\
Helvetica-Narrow-Bold:-Adobe-Helvetica-Bold-R-Narrow--\n\
Helvetica-Narrow-BoldOblique:-Adobe-Helvetica-Bold-O-Narrow--\n\
Helvetica-Narrow-Oblique:-Adobe-Helvetica-Medium-O-Narrow--\n\
Helvetica-Oblique:-Adobe-Helvetica-Medium-O-Normal--\n\
NewCenturySchlbk-Bold:-Adobe-New Century Schoolbook-Bold-R-Normal--\n\
NewCenturySchlbk-BoldItalic:-Adobe-New Century Schoolbook-Bold-I-Normal--\n\
NewCenturySchlbk-Italic:-Adobe-New Century Schoolbook-Medium-I-Normal--\n\
NewCenturySchlbk-Roman:-Adobe-New Century Schoolbook-Medium-R-Normal--\n\
Palatino-Bold:-Adobe-Palatino-Bold-R-Normal--\n\
Palatino-BoldItalic:-Adobe-Palatino-Bold-I-Normal--\n\
Palatino-Italic:-Adobe-Palatino-Medium-I-Normal--\n\
Palatino-Roman:-Adobe-Palatino-Medium-R-Normal--\n\
Times-Bold:-Adobe-Times-Bold-R-Normal--\n\
Times-BoldItalic:-Adobe-Times-Bold-I-Normal--\n\
Times-Italic:-Adobe-Times-Medium-I-Normal--\n\
Times-Roman:-Adobe-Times-Medium-R-Normal--\n\
Utopia-Bold:-Adobe-Utopia-Bold-R-Normal--\n\
Utopia-BoldItalic:-Adobe-Utopia-Bold-I-Normal--\n\
Utopia-Italic:-Adobe-Utopia-Regular-I-Normal--\n\
Utopia-Regular:-Adobe-Utopia-Regular-R-Normal--\n\
ZapfChancery-MediumItalic:-Adobe-ITC Zapf Chancery-Medium-I-Normal--"),

    rstr("symbolFonts", "SymbolFonts", symbolFonts,
	 "Symbol: -Adobe-Symbol-Medium-R-Normal--"),

    rbool("useBackingPixmap", "UseBackingPixmap", useBackingPixmap, True),
    rbool("useExternalFonts", "UseExternalFonts", useXFonts, True),
    rbool("useFontExtensions", "UseFontExtensions", useFontExtensions, True),
    rbool("useScalableFonts", "UseScalableFonts", useScalableFonts, True),
    rbool("useXPutImage", "UseXPutImage", useXPutImage, True),
    rbool("useXSetTile", "UseXSetTile", useXSetTile, True),
    rfloat("xResolution", "Resolution", xResolution, "0.0"),
    rfloat("yResolution", "Resolution", yResolution, "0.0"),

#undef RINIT
#undef rpix
#undef rdim
#undef rstr
#undef rint
#undef rbool
#undef rfloat
};

const int gdev_x_resource_count = XtNumber(gdev_x_resources);

String gdev_x_fallback_resources[] =
{
    (String) "Ghostscript*Background: white",
    (String) "Ghostscript*Foreground: black",
    NULL
};
