typedef struct Place Place;

struct Place {
	double	lon;
	double 	lat;
};
#pragma	varargck	type	"L"	Place

enum {
	Undef		= 0x80000000,
	Baud=		4800,		/* 4800 is NMEA standard speed */
};

extern Place nowhere;
extern int debug;

int placeconv(Fmt*);
Place strtopos(char*, char**);
int strtolatlon(char*, char**, Place*);
