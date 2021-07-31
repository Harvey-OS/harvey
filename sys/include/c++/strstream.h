/*ident	"@(#)cls4:incl-master/const-headers/strstream.h	1.1" */
/*******************************************************************************
 
C++ source for the C++ Language System, Release 3.0.  This product
is a new release of the original cfront developed in the computer
science research center of AT&T Bell Laboratories.

Copyright (c) 1991 AT&T and UNIX System Laboratories, Inc.
Copyright (c) 1984, 1989, 1990 AT&T.  All Rights Reserved.

THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE of AT&T and UNIX System
Laboratories, Inc.  The copyright notice above does not evidence
any actual or intended publication of such source code.

*******************************************************************************/
#ifndef STRSTREAMH
#define STRSTREAMH

#ifndef IOSTREAMH
#include <iostream.h>
#endif

class strstreambuf : public streambuf
{
public: 
			strstreambuf() ;
			strstreambuf(int) ;
			strstreambuf(void* (*a)(long), void (*f)(void*)) ;
			strstreambuf(char* b, int size, char* pstart = 0 ) ;
			strstreambuf(unsigned char* b, int size, unsigned char* pstart = 0 ) ;
	int		pcount();
	void		freeze(int n=1) ;
	char*		str() ;
			~strstreambuf() ;

public: /* virtuals  */
	virtual int	doallocate() ;
	virtual int	overflow(int) ;
	virtual int	underflow() ;
	virtual streambuf*
			setbuf(char*  p, int l) ;
	virtual streampos
			seekoff(streamoff,ios::seek_dir,int) ;

private:
	void		init(char*,int,char*) ;

	void*		(*afct)(long) ;
	void		(*ffct)(void*) ;
	int		ignore_oflow ;
	int		froozen ;
	int		auto_extend ;

public:
	int		isfrozen() { return froozen; }
	} ;

class strstreambase : public virtual ios {
public:
	strstreambuf*	rdbuf() ;
protected:	
			strstreambase(char*, int, char*) ;
			strstreambase() ;
			~strstreambase() ;
private:
	strstreambuf	buf ; 
	} ;

class istrstream : public strstreambase, public istream {
public:
			istrstream(char* str);
			istrstream(char* str, int size ) ;
			istrstream(const char* str);
			istrstream(const char* str, int size);
			~istrstream() ;
	} ;

class ostrstream : public strstreambase, public ostream {
public:
			ostrstream(char* str, int size, int=ios::out) ;
			ostrstream() ;
			~ostrstream() ;
	char*		str() ;
	int		pcount() ;
	} ;


class strstream : public strstreambase, public iostream {
public:
			strstream() ;
			strstream(char* str, int size, int mode) ;
			~strstream() ;
	char*		str() ;
	} ;

#endif
