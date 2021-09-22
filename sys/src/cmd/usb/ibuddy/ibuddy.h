typedef struct Ibuddy Ibuddy;

enum{
	/* movement direction is relative to the creature */
	IBRight,   
	IBLeft,	 
	IBClose,
	IBOpen,
	IBRed,
	IBGreen,		
	IBBlue,
	IBHeart,
	NIBactions,

	IBOff = 0xff ,

	IBFlaptime = 200, 	/* ms */
	IBTwisttime = 500, 	/* ms */

	Ibuddyvid = 0x1130,	/* Vendor ID */
	Ibuddydid = 0x0005,	/* Device ID */
	
};


/* head li*/
enum{
	IBLightHeadRed,
	IBLightHeadBlue,
	IBLightHeadGreen,
	IBLightHeart,
};

/* the ibuddy commands are sent to the control end point.
It has two other (interrupt) endpoints , what for? */

struct Ibuddy
{
	QLock;
	Dev*	dev;		/* usb device*/
	Dev*	epin1;		/* ?? */
	Dev*	epin2;		/* ?? */
	
	/* here should be the status */
	uchar status;
	Usbfs	fs;
};

int ibuddymain(Dev *d, int argc, char*argv[]);

extern int ibuddydebug;

#define	dsprint	if(ibuddydebug)fprint

