/* A rle file consists of any number of:
	`scan_line's, each of which has the form:
		(short) bytes_mny, followed by bytes_mny bytes in the form:
   			(packed short) runs_mny
				if >0, then followed by runs_mny runs
				else if ==0, then followed by:
					(packed short) blank_lines_mny skipped, -1
					(packed short) runs_mny (>0), then...
						...runs_mny runs
   A `run' is two (packed short) oxs,oxe which are
	both offset counts >=0 from the prior `?x?' (starts at 0 at left margin).
	Suppose that xs, xe are the corresponding accumulated pixel indices,
	then xs is the pixel index of the first black pixel of the run,
	and xe the pixel index of the first white pixel following the run.
   A `packed short' is a byte if its value is <128, else two bytes `HIGH' & `LOW'
	with the 0200 bit of its HIGH byte set.
   */

#define HIGH(A) ((A>>8)&0177)
#define LOW(A) (A&0377)

/* run-length-encoding constants, typedefs */

/* The following assumes a worst case page width of 17 inches (Legal page, >ISO A2)
   and worst case digitizing resolution of 400 pixels/inch (e.g. CCITT Group 4),
   for a maximum of 6800 pixels/line */
#define RLE_RUNS 3401	/* maximum no. runs in a line */
#define RLE_BYTES 6800	/* maximum no. data bytes in a rle line (enough?) */

typedef struct RLE_Run {
	short xs;	/* x-coord of first pixel in run */
	short xe;	/* x-coord of last pixel in run (NOT first following) */
	} RLE_Run;
#define Init_RLE_Run {0,0}

typedef struct RLE_Yrun {
	short y,xs,xe;
	} RLE_Yrun;
#define Init_RLE_Yrun {0,0,0}

typedef struct RLE_Line {
	short y;	/* y-coord of line */
	short len;	/* length of line in pixels (white+black) */
	short runs;	/* no. of runs */
	RLE_Run r[RLE_RUNS];
	} RLE_Line;
#define Init_RLE_Line {0,0,0,Init_RLE_Run}

typedef struct RLE_Lines {
	int mny;
	RLE_Line *rla;	/* array of RLE_Lines */
	} RLE_Lines;
#define Init_RLE_Lines {0,NULL}
extern RLE_Lines empty_RLE_Lines;

typedef struct Transform_rlel_arg {
	boolean ident;	/* if T, then no change (speed-optimization) */
	Bbx tr;		/* trim:  select just this window of input */
	Sp off;		/* offset:  translate by off.x,off.y */
	Pp scl;		/* scale:  X & Y expansion factors (about 0,0) */
	Sp wh;		/* truncate:  exact maximum output width,height */
	Radians rot;	/* rotate:  angle (multiple of PI/4) */
	int sy;		/* next integer line no. to write */
	double dy;	/* next real line no. to write */
	} Transform_rlel_arg;
#define Init_Transform_rlel_arg {T,Init_Bbx,Init_Zero_Sp,{1.0,1.0},Init_Zero_Sp,0.0,0,0.0}
extern Transform_rlel_arg empty_Transform_rlel_arg;

int rlbr(char [],int,short []);
