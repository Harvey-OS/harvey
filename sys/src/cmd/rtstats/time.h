typedef vlong Time;
typedef uvlong Ticks;

#pragma	varargck	type	"T"		vlong

extern	Ticks	fasthz;

int		timeconv(Fmt*);
Time		ticks2time(Ticks);
Ticks	time2ticks(Time);
char *	parsetime(Time *rt, char *s);
