/*	if system V #define SYS_V should be after #undef */
#define SYS_V
#undef SYS_V
/*	if system BSD #define BSD should be after #undef */
#define BSD
#undef BSD
/*	if vax bit/byte order #define VAX should be after #undef */
#define VAX
#undef VAX
/*	if ibm bit/byte order #define AHMDAL should be after #undef mips is ibm order*/
#undef AHMDAL
#define AHMDAL

/*	if sun bit/byte order #define SUN should be after #undef */
/*	for the terminal version also define XWINDOWS below*/
#define SUN
#undef SUN

/*	if compiling on 386 define I386 to avoid compiler bugs */
/*	this is only for the mainframe version */
/*	bit/byte order must also be defined - define VAX above*/
#define I386
#undef I386
/*	defined for plan9 - mainframe with mouse code*/
#undef Plan9
#define Plan9
/*	if compiling mainframe version at fax 200 dpi resolution #define after #undef*/
/*		if SINGLE not defined, multipage fax mhs format g31 output */
/*		if SINGLE is defined, single page g31 output(see below)*/
#define FAX
#undef FAX
/*	HIRES for 300dpi version and fax g4 output single page */
#define HIRES
#undef HIRES

#ifdef HIRES
#define SINGLE
#undef Plan9
#else
#undef SINGLE
#endif
/*		DON'T TOUCH undef's BELOW	*/
#ifdef makefonts
#undef XWINDOWS
#undef FAX
#undef HIRES
#undef Plan9
#endif
