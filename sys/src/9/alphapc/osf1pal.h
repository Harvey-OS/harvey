/*
 * OSF/1 PALcode instructions, in numerical order.
 * Values are from Digital EBSDK and FreeBSD/Alpha.
 */

/* Privilaged PAL functions */
#define	PALhalt		0x00	/* required per Alpha architecture */
#define	PALcflush	0x01
#define	PALdraina	0x02	/* required per Alpha architecture */
/*
 * ... 0x03 to 0x08 ?
 */
#define	PALcserve	0x09
#define	PALswppal	0x0a
/*
 * ... 0x0b to 0x0c ?
 */
#define	PALwripir	0x0d
/*
 * ... 0x0e to 0x0f ?
 */
#define	PALrdmces	0x10
#define	PALwrmces	0x11
/*
 * ... 0x12 to 0x2a ?
 */
#define	PALwrfen	0x2b
				/* 0x2c OSF/1 ? */
#define	PALwrvptptr	0x2d
/*
 * ... 0x2e to 0x2f ?
 */
#define	PALswpctx	0x30
#define	PALwrval	0x31
#define	PALrdval	0x32
#define	PALtbi		0x33
#define	PALwrent	0x34
#define	PALswpipl	0x35
#define	PALrdps		0x36
#define	PALwrkgp	0x37
#define	PALwrusp	0x38
#define	PALwrperfmon	0x39
#define	PALrdusp	0x3a
				/* 0x3b OSF/1 ? */
#define	PALwhami	0x3c
#define	PALretsys	0x3d
#define	PALwtint	0x3e
#define	PALrti		0x3f

/* Unprivileged PAL functions */
#define	PALbpt		0x80
#define	PALbugchk	0x81
#define	PALcallsys	0x83
#define	PALimb		0x86	/* required per Alpha architecture */
/*
 * ... 0x89 to 0x91 ?
 */
#define	PALurti		0x92
/*
 * ... 0x93 to 0x9d ?
 */
#define	PALrdunique	0x9e
#define	PALwrunique	0x9f
/*
 * ... 0xa0 to 0xa9 ?
 */
#define	PALgentrap	0xaa
/*
 * ... 0xab to 0xac ?
 */
#define	PALdbgstop	0xad
#define	PALclrfen	0xae
/*
 * ... 0xaf to 0xbd ?
 */
#define	PALnphalt	0xbe
#define	PALcopypal	0xbf

