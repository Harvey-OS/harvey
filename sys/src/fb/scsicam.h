/*
 * scsicam info.
 */
#define	SCAMDEPTH	2
#define	BPP	(1<<SCAMDEPTH)		/* bits/pixel, version 2 of scsicam */

#define DX	256
#define DY	240

#define GOODDY	(DY-2)			/* the bottom 1.5 rows are usually black */

#define	CPR	((DX*BPP)/8)		/* bytes per row */
#define FRAMESIZE	(DY*CPR)	/* bytes per frame */
#define PM	((1<<BPP)-1)		/* pixel mask */

#define FIRSTDATA(x,y)	(buf + CPR*y*DIVISIONS + CPD*x)
