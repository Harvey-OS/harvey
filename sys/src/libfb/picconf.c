#include <u.h>
#include <libg.h>
#include <fb.h>
int _PRDpl9(PICFILE *, void *), _PWRpl9(PICFILE *, void *), _PCLpl9(PICFILE *);
int _PRDdmp(PICFILE *, void *), _PWRdmp(PICFILE *, void *), _PCLdmp(PICFILE *);
int _PRDrun(PICFILE *, void *), _PWRrun(PICFILE *, void *), _PCLrun(PICFILE *);
int _PRDbit(PICFILE *, void *), _PWRbit(PICFILE *, void *), _PCLbit(PICFILE *);
int _PRDg31(PICFILE *, void *), _PWRg31(PICFILE *, void *), _PCLg31(PICFILE *);
int _PRDg32(PICFILE *, void *), _PWRg32(PICFILE *, void *), _PCLg32(PICFILE *);
int _PRDg4(PICFILE *, void *),  _PWRg4(PICFILE *, void *),  _PCLg4(PICFILE *);
int _PRD601(PICFILE *, void *), _PWR601(PICFILE *, void *), _PCL601(PICFILE *);
int _PRDpco(PICFILE *, void *), _PWRpco(PICFILE *, void *), _PCLpco(PICFILE *);
struct _PICconf _PICconf[]={
	"dump",		_PRDdmp,	_PWRdmp,	_PCLdmp,	0,
	"runcode",	_PRDrun,	_PWRrun,	_PCLrun,	0,
	"bitmap",	_PRDbit,	_PWRbit,	_PCLbit,	1,
	"pico",		_PRDpco,	_PWRpco,	_PCLpco,	0,
	"plan9",	_PRDpl9,	_PWRpl9,	_PCLpl9,	1,
#ifdef NOTYET
	"ccitt-g31",	_PRDg31,	_PWRg31,	_PCLg31,	1,
	"ccitt-g32",	_PRDg32,	_PWRg32,	_PCLg32,	1,
	"ccitt-g4",	_PRDg4,		_PWRg4,		_PCLg4,		1,
	"ccir601",	_PRD601,	_PWR601,	_PCL601,	1,
#endif
	0
};
