/*
 * USB keyboard/mouse constants
 */
enum {

	Stack = 32 * 1024,

	/* HID class subclass protocol ids */
	PtrCSP		= 0x020103,	/* mouse.boot.hid */
	HidCSP		= 0x000003,	/* could be a trackpoint */
	PtrNonBootCSP	= 0x020003,
	KbdCSP		= 0x010103,	/* keyboard.boot.hid */

	/* Requests */
	Getreport = 0x01,
	Setreport = 0x09,
	Getproto	= 0x03,
	Setidle		= 0x0a,
	Setproto	= 0x0b,

	/* protocols for SET_PROTO request */
	Bootproto	= 0,
	Reportproto	= 1,

	/* protocols for SET_REPORT request */
	Reportout = 0x0200,
};

/*
 * USB HID report descriptor item tags
 */ 
enum {
	/* main items */
	Input	= 8,
	Output,
	Collection,
	Feature,

	CollectionEnd,

	/* global items */
	UsagPg = 0,
	LogiMin,
	LogiMax,
	PhysMin,
	PhysMax,
	UnitExp,
	UnitTyp,
	RepSize,
	RepId,
	RepCnt,

	Push,
	Pop,

	/* local items */
	Usage	= 0,
	UsagMin,
	UsagMax,
	DesgIdx,
	DesgMin,
	DesgMax,
	StrgIdx,
	StrgMin,
	StrgMax,

	Delim,
};

/* main item flags */
enum {
	Fdata	= 0<<0,	Fconst	= 1<<0,
	Farray	= 0<<1,	Fvar	= 1<<1,
	Fabs	= 0<<2,	Frel	= 1<<2,
	Fnowrap	= 0<<3,	Fwrap	= 1<<3,
	Flinear	= 0<<4,	Fnonlin	= 1<<4,
	Fpref	= 0<<5,	Fnopref	= 1<<5,
	Fnonull	= 0<<6,	Fnullst	= 1<<6,
};
