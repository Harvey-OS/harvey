/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <ctype.h>

unsigned char	_ctype[] =
{
/* 	0			1			2			3  */
/* 	4			5			6			7  */

/*  0*/	_IScntrl,		_IScntrl,		_IScntrl,		_IScntrl,
	_IScntrl,		_IScntrl,		_IScntrl,		_IScntrl,
/* 10*/	_IScntrl,		_ISspace|_IScntrl,	_ISspace|_IScntrl,	_ISspace|_IScntrl,
	_ISspace|_IScntrl,	_ISspace|_IScntrl,	_IScntrl,		_IScntrl,
/* 20*/	_IScntrl,		_IScntrl,		_IScntrl,		_IScntrl,
	_IScntrl,		_IScntrl,		_IScntrl,		_IScntrl,
/* 30*/	_IScntrl,		_IScntrl,		_IScntrl,		_IScntrl,
	_IScntrl,		_IScntrl,		_IScntrl,		_IScntrl,
/* 40*/	_ISspace|_ISblank,	_ISpunct,		_ISpunct,		_ISpunct,
	_ISpunct,		_ISpunct,		_ISpunct,		_ISpunct,
/* 50*/	_ISpunct,		_ISpunct,		_ISpunct,		_ISpunct,
	_ISpunct,		_ISpunct,		_ISpunct,		_ISpunct,
/* 60*/	_ISdigit|_ISxdigit,	_ISdigit|_ISxdigit,	_ISdigit|_ISxdigit,	_ISdigit|_ISxdigit,
	_ISdigit|_ISxdigit,	_ISdigit|_ISxdigit,	_ISdigit|_ISxdigit,	_ISdigit|_ISxdigit,
/* 70*/	_ISdigit|_ISxdigit,	_ISdigit|_ISxdigit,	_ISpunct,		_ISpunct,
	_ISpunct,		_ISpunct,		_ISpunct,		_ISpunct,
/*100*/	_ISpunct,		_ISupper|_ISxdigit,	_ISupper|_ISxdigit,	_ISupper|_ISxdigit,
	_ISupper|_ISxdigit,	_ISupper|_ISxdigit,	_ISupper|_ISxdigit,	_ISupper,
/*110*/	_ISupper,		_ISupper,		_ISupper,		_ISupper,
	_ISupper,		_ISupper,		_ISupper,		_ISupper,
/*120*/	_ISupper,		_ISupper,		_ISupper,		_ISupper,
	_ISupper,		_ISupper,		_ISupper,		_ISupper,
/*130*/	_ISupper,		_ISupper,		_ISupper,		_ISpunct,
	_ISpunct,		_ISpunct,		_ISpunct,		_ISpunct,
/*140*/	_ISpunct,		_ISlower|_ISxdigit,	_ISlower|_ISxdigit,	_ISlower|_ISxdigit,
	_ISlower|_ISxdigit,	_ISlower|_ISxdigit,	_ISlower|_ISxdigit,	_ISlower,
/*150*/	_ISlower,		_ISlower,		_ISlower,		_ISlower,
	_ISlower,		_ISlower,		_ISlower,		_ISlower,
/*160*/	_ISlower,		_ISlower,		_ISlower,		_ISlower,
	_ISlower,		_ISlower,		_ISlower,		_ISlower,
/*170*/	_ISlower,		_ISlower,		_ISlower,		_ISpunct,
	_ISpunct,		_ISpunct,		_ISpunct,		_IScntrl,
/*200*/	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,
	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,
	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,
	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,
	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,
	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,
	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,
	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,
	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,
	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,
	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,
	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,
	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,
	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,
	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,
	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0
};
