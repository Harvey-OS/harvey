/*
 * USB keyboard/mouse constants
 */
enum {

	Stack = 32 * 1024,

	/* HID class subclass protocol ids */
	PtrCSP		= 0x020103,	/* mouse.boot.hid */
	KbdCSP		= 0x010103,	/* keyboard.boot.hid */

	/* Requests */
	Getproto	= 0x03,
	Setproto	= 0x0b,

	/* protocols for SET_PROTO request */
	Bootproto	= 0,
	Reportproto	= 1,
};

enum {
	/* keyboard modifier bits */
	Mlctrl		= 0,
	Mlshift		= 1,
	Mlalt		= 2,
	Mlgui		= 3,
	Mrctrl		= 4,
	Mrshift		= 5,
	Mralt		= 6,
	Mrgui		= 7,

	/* masks for byte[0] */
	Mctrl		= 1<<Mlctrl | 1<<Mrctrl,
	Mshift		= 1<<Mlshift | 1<<Mrshift,
	Malt		= 1<<Mlalt | 1<<Mralt,
	Mcompose	= 1<<Mlalt,
	Maltgr		= 1<<Mralt,
	Mgui		= 1<<Mlgui | 1<<Mrgui,

	MaxAcc = 3,			/* max. ptr acceleration */
	PtrMask= 0xf,			/* 4 buttons: should allow for more. */

};

/*
 * Plan 9 keyboard driver constants.
 */
enum {
	/* Scan codes (see kbd.c) */
	SCesc1		= 0xe0,		/* first of a 2-character sequence */
	SCesc2		= 0xe1,
	SClshift		= 0x2a,
	SCrshift		= 0x36,
	SCctrl		= 0x1d,
	SCcompose	= 0x38,
	Keyup		= 0x80,		/* flag bit */
	Keymask		= 0x7f,		/* regular scan code bits */
};

int kbmain(Dev *d, int argc, char*argv[]);
