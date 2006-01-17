/* Copyright (C) 1995, 1997, 1998, 2001 Aladdin Enterprises.  All rights reserved.
  
  This software is provided AS-IS with no warranty, either express or
  implied.
  
  This software is distributed under license and may not be copied,
  modified or distributed except as expressly authorized under the terms
  of the license contained in the file LICENSE in this distribution.
  
  For more information about licensing, please refer to
  http://www.ghostscript.com/licensing/. For information on
  commercial licensing, go to http://www.artifex.com/licensing/ or
  contact Artifex Software, Inc., 101 Lucas Valley Road #110,
  San Rafael, CA  94903, U.S.A., +1(415)492-9861.
*/

/* $Id: gsroptab.c,v 1.5 2002/02/21 22:24:52 giles Exp $ */
/* Table of RasterOp procedures */
#include "stdpre.h"
#include "gsropt.h"

/*
 * The H-P documentation (probably copied from Microsoft documentation)
 * specifies RasterOp algorithms using reverse Polish notation, with
 *   a = AND, o = OR, n = NOT, x = XOR
 */

#define ROP_PROC(pname, expr)\
private rop_operand pname(rop_operand D, rop_operand S, rop_operand T)\
{ return expr; }

#define a(u,v) (u&v)
#define o(u,v) (u|v)
#define x(u,v) (u^v)

ROP_PROC(rop0, 0)		/* 0 */
ROP_PROC(rop1, ~(D | S | T))		/* DTSoon */
ROP_PROC(rop2, D & ~(S | T))		/* DTSona */
ROP_PROC(rop3, ~(S | T))	/* TSon */
ROP_PROC(rop4, S & ~(D | T))		/* SDTona */
ROP_PROC(rop5, ~(D | T))	/* DTon */
ROP_PROC(rop6, ~(T | ~(D ^ S)))		/* TDSxnon */
ROP_PROC(rop7, ~(T | (D & S)))		/* TDSaon */
ROP_PROC(rop8, S & (D & ~T))		/* SDTnaa */
ROP_PROC(rop9, ~(T | (D ^ S)))		/* TDSxon */
ROP_PROC(rop10, D & ~T)		/* DTna */
ROP_PROC(rop11, ~(T | (S & ~D)))	/* TSDnaon */
ROP_PROC(rop12, S & ~T)		/* STna */
ROP_PROC(rop13, ~(T | (D & ~S)))	/* TDSnaon */
ROP_PROC(rop14, ~(T | ~(D | S)))	/* TDSonon */
ROP_PROC(rop15, ~T)		/* Tn */
ROP_PROC(rop16, T & ~(D | S))		/* TDSona */
ROP_PROC(rop17, ~(D | S))	/* DSon */
ROP_PROC(rop18, ~(S | ~(D ^ T)))	/* SDTxnon */
ROP_PROC(rop19, ~(S | (D & T)))		/* SDTaon */
ROP_PROC(rop20, ~(D | ~(T ^ S)))	/* DTSxnon */
ROP_PROC(rop21, ~(D | (T & S)))		/* DTSaon */
ROP_PROC(rop22, (T ^ (S ^ (D & ~(T & S)))))		/* TSDTSanaxx */
ROP_PROC(rop23, ~(S ^ ((S ^ T) & (D ^ S))))		/* SSTxDSxaxn */
ROP_PROC(rop24, (S ^ T) & (T ^ D))	/* STxTDxa */
ROP_PROC(rop25, ~(S ^ (D & ~(T & S))))		/* SDTSanaxn */
ROP_PROC(rop26, T ^ (D | (S & T)))	/* TDSTaox */
ROP_PROC(rop27, ~(S ^ (D & (T ^ S))))		/* SDTSxaxn */
ROP_PROC(rop28, T ^ (S | (D & T)))	/* TSDTaox */
ROP_PROC(rop29, ~(D ^ (S & (T ^ D))))		/* DSTDxaxn */
ROP_PROC(rop30, T ^ (D | S))		/* TDSox */
ROP_PROC(rop31, ~(T & (D | S)))		/* TDSoan */
ROP_PROC(rop32, D & (T & ~S))		/* DTSnaa */
ROP_PROC(rop33, ~(S | (D ^ T)))		/* SDTxon */
ROP_PROC(rop34, D & ~S)		/* DSna */
ROP_PROC(rop35, ~(S | (T & ~D)))	/* STDnaon */
ROP_PROC(rop36, (S ^ T) & (D ^ S))	/* STxDSxa */
ROP_PROC(rop37, ~(T ^ (D & ~(S & T))))		/* TDSTanaxn */
ROP_PROC(rop38, S ^ (D | (T & S)))	/* SDTSaox */
ROP_PROC(rop39, S ^ (D | ~(T ^ S)))		/* SDTSxnox */
ROP_PROC(rop40, D & (T ^ S))		/* DTSxa */
ROP_PROC(rop41, ~(T ^ (S ^ (D | (T & S)))))		/* TSDTSaoxxn */
ROP_PROC(rop42, D & ~(T & S))		/* DTSana */
ROP_PROC(rop43, ~x(a(x(D, T), x(T, S)), S))		/* SSTxTDxaxn */
ROP_PROC(rop44, (S ^ (T & (D | S))))		/* STDSoax */
ROP_PROC(rop45, T ^ (S | ~D))		/* TSDnox */
ROP_PROC(rop46, (T ^ (S | (D ^ T))))		/* TSDTxox */
ROP_PROC(rop47, ~(T & (S | ~D)))	/* TSDnoan */
ROP_PROC(rop48, T & ~S)		/* TSna */
ROP_PROC(rop49, ~(S | (D & ~T)))	/* SDTnaon */
ROP_PROC(rop50, S ^ (D | (T | S)))	/* SDTSoox */
ROP_PROC(rop51, ~S)		/* Sn */
ROP_PROC(rop52, S ^ (T | (D & S)))	/* STDSaox */
ROP_PROC(rop53, S ^ (T | ~(D ^ S)))		/* STDSxnox */
ROP_PROC(rop54, S ^ (D | T))		/* SDTox */
ROP_PROC(rop55, ~(S & (D | T)))		/* SDToan */
ROP_PROC(rop56, T ^ (S & (D | T)))	/* TSDToax */
ROP_PROC(rop57, S ^ (T | ~D))		/* STDnox */
ROP_PROC(rop58, S ^ (T | (D ^ S)))	/* STDSxox */
ROP_PROC(rop59, ~(S & (T | ~D)))	/* STDnoan */
ROP_PROC(rop60, T ^ S)		/* TSx */
ROP_PROC(rop61, S ^ (T | ~(D | S)))		/* STDSonox */
ROP_PROC(rop62, S ^ (T | (D & ~S)))		/* STDSnaox */
ROP_PROC(rop63, ~(T & S))	/* TSan */
ROP_PROC(rop64, T & (S & ~D))		/* TSDnaa */
ROP_PROC(rop65, ~(D | (T ^ S)))		/* DTSxon */
ROP_PROC(rop66, (S ^ D) & (T ^ D))	/* SDxTDxa */
ROP_PROC(rop67, ~(S ^ (T & ~(D & S))))		/* STDSanaxn */
ROP_PROC(rop68, S & ~D)		/* SDna */
ROP_PROC(rop69, ~(D | (T & ~S)))	/* DTSnaon */
ROP_PROC(rop70, D ^ (S | (T & D)))	/* DSTDaox */
ROP_PROC(rop71, ~(T ^ (S & (D ^ T))))		/* TSDTxaxn */
ROP_PROC(rop72, S & (D ^ T))		/* SDTxa */
ROP_PROC(rop73, ~(T ^ (D ^ (S | (T & D)))))		/* TDSTDaoxxn */
ROP_PROC(rop74, D ^ (T & (S | D)))	/* DTSDoax */
ROP_PROC(rop75, T ^ (D | ~S))		/* TDSnox */
ROP_PROC(rop76, S & ~(D & T))		/* SDTana */
ROP_PROC(rop77, ~(S ^ ((S ^ T) | (D ^ S))))		/* SSTxDSxoxn */
ROP_PROC(rop78, T ^ (D | (S ^ T)))	/* TDSTxox */
ROP_PROC(rop79, ~(T & (D | ~S)))	/* TDSnoan */
ROP_PROC(rop80, T & ~D)		/* TDna */
ROP_PROC(rop81, ~(D | (S & ~T)))	/* DSTnaon */
ROP_PROC(rop82, D ^ (T | (S & D)))	/* DTSDaox */
ROP_PROC(rop83, ~(S ^ (T & (D ^ S))))		/* STDSxaxn */
ROP_PROC(rop84, ~(D | ~(T | S)))	/* DTSonon */
ROP_PROC(rop85, ~D)		/* Dn */
ROP_PROC(rop86, D ^ (T | S))		/* DTSox */
ROP_PROC(rop87, ~(D & (T | S)))		/* DTSoan */
ROP_PROC(rop88, T ^ (D & (S | T)))	/* TDSToax */
ROP_PROC(rop89, D ^ (T | ~S))		/* DTSnox */
ROP_PROC(rop90, D ^ T)		/* DTx */
ROP_PROC(rop91, D ^ (T | ~(S | D)))		/* DTSDonox */
ROP_PROC(rop92, D ^ (T | (S ^ D)))	/* DTSDxox */
ROP_PROC(rop93, ~(D & (T | ~S)))	/* DTSnoan */
ROP_PROC(rop94, D ^ (T | (S & ~D)))		/* DTSDnaox */
ROP_PROC(rop95, ~(D & T))	/* DTan */
ROP_PROC(rop96, T & (D ^ S))		/* TDSxa */
ROP_PROC(rop97, ~(D ^ (S ^ (T | (D & S)))))		/* DSTDSaoxxn */
ROP_PROC(rop98, D ^ (S & (T | D)))	/* DSTDoax */
ROP_PROC(rop99, S ^ (D | ~T))		/* SDTnox */
ROP_PROC(rop100, S ^ (D & (T | S)))		/* SDTSoax */
ROP_PROC(rop101, D ^ (S | ~T))		/* DSTnox */
ROP_PROC(rop102, D ^ S)		/* DSx */
ROP_PROC(rop103, S ^ (D | ~(T | S)))		/* SDTSonox */
ROP_PROC(rop104, ~(D ^ (S ^ (T | ~(D | S)))))		/* DSTDSonoxxn */
ROP_PROC(rop105, ~(T ^ (D ^ S)))	/* TDSxxn */
ROP_PROC(rop106, D ^ (T & S))		/* DTSax */
ROP_PROC(rop107, ~(T ^ (S ^ (D & (T | S)))))		/* TSDTSoaxxn */
ROP_PROC(rop108, (D & T) ^ S)		/* SDTax */
ROP_PROC(rop109, ~((((T | D) & S) ^ D) ^ T))		/* TDSTDoaxxn */
ROP_PROC(rop110, ((~S | T) & D) ^ S)		/* SDTSnoax */
ROP_PROC(rop111, ~(~(D ^ S) & T))	/* TDSxnan */
ROP_PROC(rop112, ~(D & S) & T)		/* TDSana */
ROP_PROC(rop113, ~(((S ^ D) & (T ^ D)) ^ S))		/* SSDxTDxaxn */
ROP_PROC(rop114, ((T ^ S) | D) ^ S)		/* SDTSxox */
ROP_PROC(rop115, ~((~T | D) & S))	/* SDTnoan */
ROP_PROC(rop116, ((T ^ D) | S) ^ D)		/* DSTDxox */
ROP_PROC(rop117, ~((~T | S) & D))	/* DSTnoan */
ROP_PROC(rop118, ((~S & T) | D) ^ S)		/* SDTSnaox */
ROP_PROC(rop119, ~(D & S))	/* DSan */
ROP_PROC(rop120, (D & S) ^ T)		/* TDSax */
ROP_PROC(rop121, ~((((D | S) & T) ^ S) ^ D))		/* DSTDSoaxxn */
ROP_PROC(rop122, ((~D | S) & T) ^ D)		/* DTSDnoax */
ROP_PROC(rop123, ~(~(D ^ T) & S))	/* SDTxnan */
ROP_PROC(rop124, ((~S | D) & T) ^ S)		/* STDSnoax */
ROP_PROC(rop125, ~(~(T ^ S) & D))	/* DTSxnan */
ROP_PROC(rop126, (S ^ T) | (D ^ S))		/* STxDSxo */
ROP_PROC(rop127, ~((T & S) & D))	/* DTSaan */
ROP_PROC(rop128, (T & S) & D)		/* DTSaa */
ROP_PROC(rop129, ~((S ^ T) | (D ^ S)))		/* STxDSxon */
ROP_PROC(rop130, ~(T ^ S) & D)		/* DTSxna */
ROP_PROC(rop131, ~(((~S | D) & T) ^ S))		/* STDSnoaxn */
ROP_PROC(rop132, ~(D ^ T) & S)		/* SDTxna */
ROP_PROC(rop133, ~(((~T | S) & D) ^ T))		/* TDSTnoaxn */
ROP_PROC(rop134, (((D | S) & T) ^ S) ^ D)	/* DSTDSoaxx */
ROP_PROC(rop135, ~((D & S) ^ T))	/* TDSaxn */
ROP_PROC(rop136, D & S)		/* DSa */
ROP_PROC(rop137, ~(((~S & T) | D) ^ S))		/* SDTSnaoxn */
ROP_PROC(rop138, (~T | S) & D)		/* DSTnoa */
ROP_PROC(rop139, ~(((T ^ D) | S) ^ D))		/* DSTDxoxn */
ROP_PROC(rop140, (~T | D) & S)		/* SDTnoa */
ROP_PROC(rop141, ~(((T ^ S) | D) ^ S))		/* SDTSxoxn */
ROP_PROC(rop142, ((S ^ D) & (T ^ D)) ^ S)	/* SSDxTDxax */
ROP_PROC(rop143, ~(~(D & S) & T))	/* TDSanan */
ROP_PROC(rop144, ~(D ^ S) & T)		/* TDSxna */
ROP_PROC(rop145, ~(((~S | T) & D) ^ S))		/* SDTSnoaxn */
ROP_PROC(rop146, (((D | T) & S) ^ T) ^ D)	/* DTSDToaxx */
ROP_PROC(rop147, ~((T & D) ^ S))	/* STDaxn */
ROP_PROC(rop148, (((T | S) & D) ^ S) ^ T)	/* TSDTSoaxx */
ROP_PROC(rop149, ~((T & S) ^ D))	/* DTSaxn */
ROP_PROC(rop150, (T ^ S) ^ D)		/* DTSxx */
ROP_PROC(rop151, ((~(T | S) | D) ^ S) ^ T)	/* TSDTSonoxx */
ROP_PROC(rop152, ~((~(T | S) | D) ^ S))		/* SDTSonoxn */
ROP_PROC(rop153, ~(D ^ S))	/* DSxn */
ROP_PROC(rop154, (~S & T) ^ D)		/* DTSnax */
ROP_PROC(rop155, ~(((T | S) & D) ^ S))		/* SDTSoaxn */
ROP_PROC(rop156, (~D & T) ^ S)		/* STDnax */
ROP_PROC(rop157, ~(((T | D) & S) ^ D))		/* DSTDoaxn */
ROP_PROC(rop158, (((D & S) | T) ^ S) ^ D)	/* DSTDSaoxx */
ROP_PROC(rop159, ~((D ^ S) & T))	/* TDSxan */
ROP_PROC(rop160, D & T)		/* DTa */
ROP_PROC(rop161, ~(((~T & S) | D) ^ T))		/* TDSTnaoxn */
ROP_PROC(rop162, (~S | T) & D)		/* DTSnoa */
ROP_PROC(rop163, ~(((D ^ S) | T) ^ D))		/* DTSDxoxn */
ROP_PROC(rop164, ~((~(T | S) | D) ^ T))		/* TDSTonoxn */
ROP_PROC(rop165, ~(D ^ T))	/* TDxn */
ROP_PROC(rop166, (~T & S) ^ D)		/* DSTnax */
ROP_PROC(rop167, ~(((T | S) & D) ^ T))		/* TDSToaxn */
ROP_PROC(rop168, ((S | T) & D))		/* DTSoa */
ROP_PROC(rop169, ~((S | T) ^ D))	/* DTSoxn */
ROP_PROC(rop170, D)		/* D */
ROP_PROC(rop171, ~(S | T) | D)		/* DTSono */
ROP_PROC(rop172, (((S ^ D) & T) ^ S))		/* STDSxax */
ROP_PROC(rop173, ~(((D & S) | T) ^ D))		/* DTSDaoxn */
ROP_PROC(rop174, (~T & S) | D)		/* DSTnao */
ROP_PROC(rop175, ~T | D)	/* DTno */
ROP_PROC(rop176, (~S | D) & T)		/* TDSnoa */
ROP_PROC(rop177, ~(((T ^ S) | D) ^ T))		/* TDSTxoxn */
ROP_PROC(rop178, ((S ^ D) | (S ^ T)) ^ S)	/* SSTxDSxox */
ROP_PROC(rop179, ~(~(T & D) & S))	/* SDTanan */
ROP_PROC(rop180, (~D & S) ^ T)		/* TSDnax */
ROP_PROC(rop181, ~(((D | S) & T) ^ D))		/* DTSDoaxn */
ROP_PROC(rop182, (((T & D) | S) ^ T) ^ D)	/* DTSDTaoxx */
ROP_PROC(rop183, ~((T ^ D) & S))	/* SDTxan */
ROP_PROC(rop184, ((T ^ D) & S) ^ T)		/* TSDTxax */
ROP_PROC(rop185, (~((D & T) | S) ^ D))		/* DSTDaoxn */
ROP_PROC(rop186, (~S & T) | D)		/* DTSnao */
ROP_PROC(rop187, ~S | D)	/* DSno */
ROP_PROC(rop188, (~(S & D) & T) ^ S)		/* STDSanax */
ROP_PROC(rop189, ~((D ^ T) & (D ^ S)))		/* SDxTDxan */
ROP_PROC(rop190, (S ^ T) | D)		/* DTSxo */
ROP_PROC(rop191, ~(S & T) | D)		/* DTSano */
ROP_PROC(rop192, T & S)		/* TSa */
ROP_PROC(rop193, ~(((~S & D) | T) ^ S))		/* STDSnaoxn */
ROP_PROC(rop194, ~x(o(~o(S, D), T), S))		/* STDSonoxn */
ROP_PROC(rop195, ~(S ^ T))	/* TSxn */
ROP_PROC(rop196, ((~D | T) & S))	/* STDnoa */
ROP_PROC(rop197, ~(((S ^ D) | T) ^ S))		/* STDSxoxn */
ROP_PROC(rop198, ((~T & D) ^ S))	/* SDTnax */
ROP_PROC(rop199, ~(((T | D) & S) ^ T))		/* TSDToaxn */
ROP_PROC(rop200, ((T | D) & S))		/* SDToa */
ROP_PROC(rop201, ~((D | T) ^ S))	/* STDoxn */
ROP_PROC(rop202, ((D ^ S) & T) ^ D)		/* DTSDxax */
ROP_PROC(rop203, ~(((S & D) | T) ^ S))		/* STDSaoxn */
ROP_PROC(rop204, S)		/* S */
ROP_PROC(rop205, ~(T | D) | S)		/* SDTono */
ROP_PROC(rop206, (~T & D) | S)		/* SDTnao */
ROP_PROC(rop207, ~T | S)	/* STno */
ROP_PROC(rop208, (~D | S) & T)		/* TSDnoa */
ROP_PROC(rop209, ~(((T ^ D) | S) ^ T))		/* TSDTxoxn */
ROP_PROC(rop210, (~S & D) ^ T)		/* TDSnax */
ROP_PROC(rop211, ~(((S | D) & T) ^ S))		/* STDSoaxn */
ROP_PROC(rop212, x(a(x(D, T), x(T, S)), S))		/* SSTxTDxax */
ROP_PROC(rop213, ~(~(S & T) & D))	/* DTSanan */
ROP_PROC(rop214, ((((S & T) | D) ^ S) ^ T))		/* TSDTS aoxx */
ROP_PROC(rop215, ~((S ^ T) & D))	/* DTS xan */
ROP_PROC(rop216, ((T ^ S) & D) ^ T)		/* TDST xax */
ROP_PROC(rop217, ~(((S & T) | D) ^ S))		/* SDTS aoxn */
ROP_PROC(rop218, x(a(~a(D, S), T), D))		/* DTSD anax */
ROP_PROC(rop219, ~a(x(S, D), x(T, S)))		/* STxDSxan */
ROP_PROC(rop220, (~D & T) | S)		/* STD nao */
ROP_PROC(rop221, ~D | S)	/* SDno */
ROP_PROC(rop222, (T ^ D) | S)		/* SDT xo */
ROP_PROC(rop223, (~(T & D)) | S)	/* SDT ano */
ROP_PROC(rop224, ((S | D) & T))		/* TDS oa */
ROP_PROC(rop225, ~((S | D) ^ T))	/*  TDS oxn */
ROP_PROC(rop226, (((D ^ T) & S) ^ D))		/* DSTD xax */
ROP_PROC(rop227, ~(((T & D) | S) ^ T))		/* TSDT aoxn */
ROP_PROC(rop228, ((S ^ T) & D) ^ S)		/* SDTSxax */
ROP_PROC(rop229, ~(((T & S) | D) ^ T))		/* TDST aoxn */
ROP_PROC(rop230, (~(S & T) & D) ^ S)		/* SDTSanax */
ROP_PROC(rop231, ~a(x(D, T), x(T, S)))		/* STxTDxan */
ROP_PROC(rop232, x(a(x(S, D), x(T, S)), S))		/* SS TxD Sxax */
ROP_PROC(rop233, ~x(x(a(~a(S, D), T), S), D))		/* DST DSan axxn   */
ROP_PROC(rop234, (S & T) | D)		/* DTSao */
ROP_PROC(rop235, ~(S ^ T) | D)		/* DTSxno */
ROP_PROC(rop236, (T & D) | S)		/* SDTao */
ROP_PROC(rop237, ~(T ^ D) | S)		/* SDTxno */
ROP_PROC(rop238, S | D)		/* DSo */
ROP_PROC(rop239, (~T | D) | S)		/* SDTnoo */
ROP_PROC(rop240, T)		/* T */
ROP_PROC(rop241, ~(S | D) | T)		/* TDSono */
ROP_PROC(rop242, (~S & D) | T)		/* TDSnao */
ROP_PROC(rop243, ~S | T)	/* TSno */
ROP_PROC(rop244, (~D & S) | T)		/* TSDnao */
ROP_PROC(rop245, ~D | T)	/* TDno */
ROP_PROC(rop246, (S ^ D) | T)		/* TDSxo */
ROP_PROC(rop247, ~(S & D) | T)		/* TDSano */
ROP_PROC(rop248, (S & D) | T)		/* TDSao */
ROP_PROC(rop249, ~(S ^ D) | T)		/* TDSxno */
ROP_PROC(rop250, D | T)		/* DTo */
ROP_PROC(rop251, (~S | T) | D)		/* DTSnoo */
ROP_PROC(rop252, S | T)		/* TSo */
ROP_PROC(rop253, (~D | S) | T)		/* TSDnoo */
ROP_PROC(rop254, S | T | D)	/* DTSoo */
ROP_PROC(rop255, ~(rop_operand) 0)	/* 1 */
#undef ROP_PROC

     const rop_proc rop_proc_table[256] = {
	 rop0, rop1, rop2, rop3, rop4, rop5, rop6, rop7,
	 rop8, rop9, rop10, rop11, rop12, rop13, rop14, rop15,
	 rop16, rop17, rop18, rop19, rop20, rop21, rop22, rop23,
	 rop24, rop25, rop26, rop27, rop28, rop29, rop30, rop31,
	 rop32, rop33, rop34, rop35, rop36, rop37, rop38, rop39,
	 rop40, rop41, rop42, rop43, rop44, rop45, rop46, rop47,
	 rop48, rop49, rop50, rop51, rop52, rop53, rop54, rop55,
	 rop56, rop57, rop58, rop59, rop60, rop61, rop62, rop63,
	 rop64, rop65, rop66, rop67, rop68, rop69, rop70, rop71,
	 rop72, rop73, rop74, rop75, rop76, rop77, rop78, rop79,
	 rop80, rop81, rop82, rop83, rop84, rop85, rop86, rop87,
	 rop88, rop89, rop90, rop91, rop92, rop93, rop94, rop95,
	 rop96, rop97, rop98, rop99, rop100, rop101, rop102, rop103,
	 rop104, rop105, rop106, rop107, rop108, rop109, rop110, rop111,
	 rop112, rop113, rop114, rop115, rop116, rop117, rop118, rop119,
	 rop120, rop121, rop122, rop123, rop124, rop125, rop126, rop127,
	 rop128, rop129, rop130, rop131, rop132, rop133, rop134, rop135,
	 rop136, rop137, rop138, rop139, rop140, rop141, rop142, rop143,
	 rop144, rop145, rop146, rop147, rop148, rop149, rop150, rop151,
	 rop152, rop153, rop154, rop155, rop156, rop157, rop158, rop159,
	 rop160, rop161, rop162, rop163, rop164, rop165, rop166, rop167,
	 rop168, rop169, rop170, rop171, rop172, rop173, rop174, rop175,
	 rop176, rop177, rop178, rop179, rop180, rop181, rop182, rop183,
	 rop184, rop185, rop186, rop187, rop188, rop189, rop190, rop191,
	 rop192, rop193, rop194, rop195, rop196, rop197, rop198, rop199,
	 rop200, rop201, rop202, rop203, rop204, rop205, rop206, rop207,
	 rop208, rop209, rop210, rop211, rop212, rop213, rop214, rop215,
	 rop216, rop217, rop218, rop219, rop220, rop221, rop222, rop223,
	 rop224, rop225, rop226, rop227, rop228, rop229, rop230, rop231,
	 rop232, rop233, rop234, rop235, rop236, rop237, rop238, rop239,
	 rop240, rop241, rop242, rop243, rop244, rop245, rop246, rop247,
	 rop248, rop249, rop250, rop251, rop252, rop253, rop254, rop255
     };

/*
 * Here is the program that generated the table below.
 *
 int
 main(int argc, char *argv[])
 {      uint i;
 for ( i = 0; i < 256; ++i )
 printf("%d,",
 (rop3_uses_D(i) ? rop_usage_D : 0) |
 (rop3_uses_S(i) ? rop_usage_S : 0) |
 (rop3_uses_T(i) ? rop_usage_T : 0));
 fflush(stdout);
 return 0;
 }
 */

     const byte /*rop_usage_t */ rop_usage_table[256] =
     {
	 0, 7, 7, 6, 7, 5, 7, 7, 7, 7, 5, 7, 6, 7, 7, 4,
	 7, 3, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
	 7, 7, 3, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
	 6, 7, 7, 2, 7, 7, 7, 7, 7, 7, 7, 7, 6, 7, 7, 6,
	 7, 7, 7, 7, 3, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
	 5, 7, 7, 7, 7, 1, 7, 7, 7, 7, 5, 7, 7, 7, 7, 5,
	 7, 7, 7, 7, 7, 7, 3, 7, 7, 7, 7, 7, 7, 7, 7, 7,
	 7, 7, 7, 7, 7, 7, 7, 3, 7, 7, 7, 7, 7, 7, 7, 7,
	 7, 7, 7, 7, 7, 7, 7, 7, 3, 7, 7, 7, 7, 7, 7, 7,
	 7, 7, 7, 7, 7, 7, 7, 7, 7, 3, 7, 7, 7, 7, 7, 7,
	 5, 7, 7, 7, 7, 5, 7, 7, 7, 7, 1, 7, 7, 7, 7, 5,
	 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 3, 7, 7, 7, 7,
	 6, 7, 7, 6, 7, 7, 7, 7, 7, 7, 7, 7, 2, 7, 7, 6,
	 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 3, 7, 7,
	 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 3, 7,
	 4, 7, 7, 6, 7, 5, 7, 7, 7, 7, 5, 7, 6, 7, 7, 0
     };
