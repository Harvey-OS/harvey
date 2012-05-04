/************** Inferno Generic Scan Conversions ************/

/* this file needs to be kept in sync with module/keyboard.m */

enum {
	Esc=		0x1b,

	Spec=		0xe000,		/* Special Function Keys, mapped to Unicode reserved range (E000-F8FF) */

	Shift=		Spec|0x0,	/* Shifter (Held and Toggle) Keys  */
	View=		Spec|0x10,	/* View Keys 		*/
	PF=		Spec|0x20,	/* num pad		*/
	KF=		Spec|0x40,	/* function keys        */

	LShift=		Shift|0,
	RShift=		Shift|1,
	LCtrl=		Shift|2,
	RCtrl=		Shift|3,
	Caps=		Shift|4,
	Num=		Shift|5,	
	Meta=		Shift|6,
	LAlt=		Shift|7,
	RAlt=		Shift|8,
	NShifts=	9,

	Home=	   	View|0,
	End=		View|1,
	Up=		View|2,
	Down=		View|3,
	Left=		View|4,
	Right=		View|5,
	Pgup=		View|6,
	Pgdown=		View|7,
	BackTab=	View|8,

	Scroll=		Spec|0x62,
	Ins=		Spec|0x63,
	Del=		Spec|0x64,
	Print=		Spec|0x65,
	Pause=		Spec|0x66,
	Middle=		Spec|0x67,
	Break=		Spec|0x66,
	SysRq=		Spec|0x69,
	PwrOn=		Spec|0x6c,
	PwrOff=		Spec|0x6d,
	PwrLow=		Spec|0x6e,
	Latin=		Spec|0x6f,

	/* for German keyboard */
	German=		Spec|0xf00,

	Grave=		German|0x1,
	Acute=		German|0x2,
	Circumflex=	German|0x3,

	APP=		Spec|0x200,		/* for ALT application keys */

	No=			-1,			/* peter */
};

