/*ident	"@(#)cls4:incl-master/const-headers/fstream.h	1.1" */
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
#ifndef FSTREAMH
#define FSTREAMH

#include <iostream.h>

class  filebuf : public streambuf {	/* a stream buffer for files */
public:
	static const int openprot ; /* default protection for open */
public:
			filebuf() ;
			filebuf(int fd);
			filebuf(int fd, char*  p, int l) ;

	int		is_open() { return opened ; }
	int		fd() { return xfd ; }
	filebuf*	open(const char *name, int om, int prot=openprot);
	filebuf*	attach(int fd) ;
	filebuf* 	close() ;
			~filebuf() ;
public: /* virtuals */
	virtual int	overflow(int=EOF);
	virtual int	underflow();
	virtual int	sync() ;
	virtual streampos
			seekoff(streamoff,ios::seek_dir,int) ;
	virtual streambuf*
			setbuf(char*  p, int len) ;
protected:
	int		xfd;	
	int		mode ;
	char		opened;
	streampos	last_seek ;
	char* 		in_start;
	int		last_op();
	char		lahead[2] ;
};

class fstreambase : virtual public ios { 
public:
			fstreambase() ;
	
			fstreambase(const char* name, 
					int mode,
					int prot=filebuf::openprot) ;
			fstreambase(int fd) ;
			fstreambase(int fd, char*  p, int l) ;
			~fstreambase() ;
	void		open(const char* name, int mode, 
					int prot=filebuf::openprot) ;
	void		attach(int fd);
	void		close() ;
	void		setbuf(char*  p, int l) ;
	filebuf*	rdbuf() { return &buf ; }
private:
	filebuf		buf ;
protected:
	void		verify(int) ;
} ;

class ifstream : public fstreambase, public istream {
public:
			ifstream() ;
			ifstream(const char* name, 
					int mode=ios::in,
					int prot=filebuf::openprot) ;
			ifstream(int fd) ;
			ifstream(int fd, char*  p, int l) ;
			~ifstream() ;

	filebuf*	rdbuf() { return fstreambase::rdbuf(); }
	void		open(const char* name, int mode=ios::in, 
					int prot=filebuf::openprot) ;
} ;

class ofstream : public fstreambase, public ostream {
public:
			ofstream() ;
			ofstream(const char* name, 
					int mode=ios::out,
					int prot=filebuf::openprot) ;
			ofstream(int fd) ;
			ofstream(int fd, char*  p, int l) ;
			~ofstream() ;

	filebuf*	rdbuf() { return fstreambase::rdbuf(); }
	void		open(const char* name, int mode=ios::out, 
					int prot=filebuf::openprot) ;
} ;

class fstream : public fstreambase, public iostream {
public:
			fstream() ;
	
			fstream(const char* name, 
					int mode,
					int prot=filebuf::openprot) ;
			fstream(int fd) ;
			fstream(int fd, char*  p, int l) ;
			~fstream() ;
	filebuf*	rdbuf() { return fstreambase::rdbuf(); }
	void		open(const char* name, int mode, 
				int prot=filebuf::openprot) ;
} ;

#endif
