enum {
	MaxScsi		= 4,
	NTarget		= 16,
	Maxnets		= 8,
};

/*
 * SCSI support code.
 */
enum {
	STblank		=-6,		/* blank block */
	STnomem		=-5,		/* buffer allocation failed */
	STtimeout	=-4,		/* bus timeout */
	STownid		=-3,		/* playing with myself */
	STharderr	=-2,		/* controller error of some kind */
	STinit		=-1,		/* */
	STok		= 0,		/* good */
	STcheck		= 0x02,		/* check condition */
	STcondmet	= 0x04,		/* condition met/good */
	STbusy		= 0x08,		/* busy */
	STintok		= 0x10,		/* intermediate/good */
	STintcondmet	= 0x14,		/* intermediate/condition met/good */
	STresconf	= 0x18,		/* reservation conflict */
	STterminated	= 0x22,		/* command terminated */
	STqfull		= 0x28,		/* queue full */
};

typedef struct Target {
	Scsi	*sc;		/* from openscsi */
	int	ctlrno;
	int	targetno;
	uchar*	inquiry;
	uchar*	sense;

	QLock;
	char	id[NAMELEN];
	int	ok;
} Target;
