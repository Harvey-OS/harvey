/* More configuration definitions for web2c.  Unlike those in site.h,
   you need not and should not change these.  If you do, then (a) tell
   the people listed in README why you need to change them, and (b) be
   prepared for everything to fail.
   
04 Feb 1990 karl
*/

/* These types can be basically anything, so they don't need to put in
   site.h.  Despite the dire warning above, probably nothing bad will
   happen if you change them -- but you shouldn't need to.  */
typedef char boolean;
typedef double real;

/* The maximum length of a filename including a directory specifier.
   Although the change files are now intended to use this symbolic
   constant, instead of repeating `1024', they might not be completely
   fixed yet.  */
#define	FILENAMESIZE	1024
