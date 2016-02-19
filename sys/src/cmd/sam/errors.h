/* 
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

typedef enum Err{
	/* error_s */
	Eopen,
	Ecreate,
	Emenu,
	Emodified,
	Eio,
	Ewseq,
	/* error_c */
	Eunk,
	Emissop,
	Edelim,
	/* error */
	Efork,
	Eintr,
	Eaddress,
	Esearch,
	Epattern,
	Enewline,
	Eblank,
	Enopattern,
	EnestXY,
	Enolbrace,
	Enoaddr,
	Eoverlap,
	Enosub,
	Elongrhs,
	Ebadrhs,
	Erange,
	Esequence,
	Eorder,
	Enoname,
	Eleftpar,
	Erightpar,
	Ebadclass,
	Ebadregexp,
	Eoverflow,
	Enocmd,
	Epipe,
	Enofile,
	Etoolong,
	Echanges,
	Eempty,
	Efsearch,
	Emanyfiles,
	Elongtag,
	Esubexp,
	Etmpovfl,
	Eappend,
	Ecantplumb,
	Ebufload,
}Err;
typedef enum Warn{
	/* warn_s */
	Wdupname,
	Wfile,
	Wdate,
	/* warn_ss */
	Wdupfile,
	/* warn */
	Wnulls,
	Wpwd,
	Wnotnewline,
	Wbadstatus,
}Warn;
