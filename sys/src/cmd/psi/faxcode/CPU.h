/* Specify computing environment.  This is an attempt to maintain
   a single master source tailorable at compile time. */

/* Specify CPU */
#define FVAX
#undef FVAX
#define FCRAY
#undef FCRAY
#define FSUN
#undef FSUN
#define FATT3B
#undef FATT3B
#define FI386
#undef FI3886
#define FAHMDAL
#undef FAHMDAL
#undef FMIPS
#define FMIPS

#ifdef SUN
#define FSUN
#else
#ifdef I386
#define FI386
#else
#ifdef AHMDAL
#define FAHMDAL
#else
#define FVAX
#endif
#endif
#endif

/* Specify operating system */
#define V9_OS		/* ATT-BL research UNIX, 9th edition */
#undef V9_OS
#define SV_OS		/* ATT UNIX System V */
#undef SV_OS
#define SUN_OS	/* Sun OS 4.x */
#undef SUN_OS
#define BSD_OS	/* Berkley */
#undef BSD_OS

#ifdef SYS_V
#define SV_OS
#else
#ifdef SUN
#define SUN_OS
#else
#ifdef BSD
#define BSD_OS
#else
#define V9_OS
#endif
#endif
#endif

/* Specify graphics display device (if any) */
#define NO_GRAPHICS	/* defeat all graphics */
#define Y_GRAPHICS	/* the Y graphics interface (used in 1127 for Metheus) */
#undef Y_GRAPHICS
#define SUN_GRAPHICS	/* Tim Thompson's Sun interface */
#undef SUN_GRAPHICS


/* Specify which fax encoding*/
/*choices G31 G32 G4*/
#ifdef FAX
#define G31
#endif
