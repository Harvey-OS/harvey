/*-----------------------------------------------------------------------*
 * These definitions cannot be defined on the command line because so many
 * DEFINE's overflow the DCL buffer when using the GNU CC compiler.
 *-----------------------------------------------------------------------*/
#ifndef TFMPATH
#define TFMPATH "TEX_FONTS:"
#endif
#ifndef PKPATH
#define PKPATH "TEX_DISK:[TEX.FONTS.%d]%f.PK"
#endif
#ifndef VFPATH
#define VFPATH  "TEX_VF:"
#endif
#ifndef FIGPATH
#define FIGPATH  ",TEX_INPUTS:,TEX$POSTSCRIPT:," /* Include blank so it looks on default dir. */
#endif
#ifndef HEADERPATH
#define HEADERPATH  "TEX$POSTSCRIPT:,SYS$LOGIN:"
#endif
#ifndef CONFIGPATH
#define CONFIGPATH "TEX$POSTSCRIPT:"
#endif
#ifndef CONFIGFILE
#define CONFIGFILE "config.ps"
#endif
#ifdef FONTLIB
#ifndef FLIPATH
#define FLIPATH ""
#endif
#ifndef FLINAME
#define FLINAME ""
#endif
#endif
