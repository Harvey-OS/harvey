/* Copyright (C) 1997-2002 artofcode LLC.  All rights reserved.                     

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

/* $Id: gdevmacpictop.h,v 1.6 2003/01/06 23:37:58 giles Exp $ */

/* Helpers for working with Classic MacOS Quickdraw pictures */
/* (obsoleted by the DISPLAY device) */

#ifndef gdevmacpictop_INCLUDED
#  define gdevmacpictop_INCLUDED

#include <QDOffscreen.h>


/************************/
/* PICT data structures */
/************************/

/* write raw data to PICT file */
#define PICTWriteByte(ptr, data)	*((unsigned char*)  (ptr))++ = data;
#define PICTWriteInt(ptr, data)		*((short*) (ptr))++ = data;
#define PICTWriteLong(ptr, data)	*((long*)  (ptr))++ = data;

#define PICTWriteFillByte(ptr)		PICTWriteByte(ptr, 0);

/* write a PICT opcode */
#define PICTWriteOpcode(ptr, op)	PICTWriteInt(ptr, op);


/*****************************/
/* QuickDraw data structures */
/*****************************/

/* write a Point structure */
#define PICTWritePoint(ptr, h, v)															\
		{																					\
			PICTWriteInt(ptr, v);	/* vertical coordinate */								\
			PICTWriteInt(ptr, h);	/* horizontal coordinate */								\
		}

/* write a Rect structure */
#define PICTWriteRect(ptr, x, y, w, h)														\
		{																					\
			PICTWritePoint(ptr, x, y);		/* upper-left corner  */						\
			PICTWritePoint(ptr, x+w, y+h);	/* lower-right corner */						\
		}

/* write a rectangular Region structure */
#define PICTWriteRegionRectangular(ptr, x, y, w, h)											\
		{																					\
			PICTWriteInt(ptr, 10);			/* rgnSize */									\
			PICTWriteRect(ptr, x, y, w, h);	/* rgnBBox */									\
		}

/* write a non-rectangular Region structure */
#define PICTWriteRegion(ptr, x, y, b, h, size, dataptr)										\
		{																					\
			PICTWriteInt(ptr, 10+size);		/* rgnSize */									\
			PICTWriteRect(ptr, x, y, w, h);	/* rgnBBox */									\
			memcpy(ptr, dataptr, size);		/* additional data */							\
			((char*)(ptr)) += size;															\
		}

/* write a pattern */
#define PICTWritePattern(ptr, byte1, byte2, byte3, byte4, byte5, byte6, byte7, byte8)		\
		{																					\
			PICTWriteByte(ptr, byte1);		/* pattern */									\
			PICTWriteByte(ptr, byte2);		/* pattern */									\
			PICTWriteByte(ptr, byte3);		/* pattern */									\
			PICTWriteByte(ptr, byte4);		/* pattern */									\
			PICTWriteByte(ptr, byte5);		/* pattern */									\
			PICTWriteByte(ptr, byte6);		/* pattern */									\
			PICTWriteByte(ptr, byte7);		/* pattern */									\
			PICTWriteByte(ptr, byte8);		/* pattern */									\
		}

/* write a RGBColor structure */
#define PICTWriteRGBColor(ptr, r, g, b)														\
		{																					\
			PICTWriteInt(ptr, r);			/* red   */										\
			PICTWriteInt(ptr, g);			/* green */										\
			PICTWriteInt(ptr, b);			/* blue  */										\
		}

/* write a ColorSpec structure */
#define PICTWriteColorSpec(ptr, value, r, g, b)														\
		{																					\
			PICTWriteInt(ptr, value);			/* value */									\
			PICTWriteRGBColor(ptr, r, g, b);	/* color */									\
		}

/* write a ColorTable structure */
#define PICTWriteColorTable(ptr, seed, numEntries, cspecarr)								\
		{																					\
			int i;																			\
			PICTWriteLong(ptr, seed);			/* ctSeed  */								\
			PICTWriteInt(ptr, 0);				/* ctFlags */								\
			PICTWriteInt(ptr, numEntries-1);	/* ctSize  */								\
			for (i=0; i<numEntries; i++)		/* ctTable */								\
				PICTWriteColorSpec(ptr, cspecarr[i].value,									\
										cspecarr[i].rgb.red,								\
										cspecarr[i].rgb.green,								\
										cspecarr[i].rgb.blue);								\
		}

/* write a PixMap structure */
#define PICTWritePixMap(ptr, x, y, w, h, rowBytes,											\
						packType, packSize,													\
						hRes, vRes, pixelSize)												\
		{																					\
			PICTWriteInt(ptr, 0x8000+rowBytes);		/* rowBytes   */						\
			PICTWriteRect(ptr, x, y, w, h);			/* bounds     */						\
			PICTWriteInt(ptr, 0);					/* pmVersion  */						\
			PICTWriteInt(ptr, packType);			/* packType   */						\
			PICTWriteLong(ptr, (packType ? packSize : 0));		/* packSize   */			\
			PICTWriteLong(ptr, hRes);				/* hRes       */						\
			PICTWriteLong(ptr, vRes);				/* vRes       */						\
			if (pixelSize < 16)	{				/* indexed */								\
				PICTWriteInt(ptr, 0);				/* pixelType  */						\
				PICTWriteInt(ptr, pixelSize);		/* pixelSize  */						\
				PICTWriteInt(ptr, 1);				/* cmpCount   */						\
				PICTWriteInt(ptr, pixelSize);		/* cmpSize    */						\
			} else {							/* direct  */								\
				PICTWriteInt(ptr, RGBDirect);		/* pixelType  */						\
				PICTWriteInt(ptr, pixelSize);		/* pixelSize  */						\
				PICTWriteInt(ptr, 3);				/* cmpCount   */						\
				PICTWriteInt(ptr, (pixelSize==16 ? 5 : 8));		/* cmpSize    */			\
			}																				\
			PICTWriteLong(ptr, 0);					/* planeBytes */						\
			PICTWriteLong(ptr, 0);					/* pmTable    */						\
			PICTWriteLong(ptr, 0);					/* pmReserved */						\
		}

/* write PackBits data */
#define PICTWriteDataPackBits(ptr, base, rowBytes, lines)									\
		{																					\
			short	byteCount;																\
			if (raster < 8) {		/* data uncompressed */									\
				byteCount = rowBytes * lines;												\
				memcpy(ptr, base, byteCount);		/* bitmap data */						\
				(char*)(ptr) += byteCount;													\
			} else {				/* raster >= 8 use PackBits compression */				\
				Ptr		destBufBegin = (Ptr) malloc(raster + (raster+126)/127), destBuf,	\
						srcBuf = (Ptr) base;												\
				short	i, len;																\
																							\
				byteCount = 0;																\
				for (i=0; i<lines; i++) {													\
					destBuf = destBufBegin;													\
					PackBits(&srcBuf, &destBuf, rowBytes);									\
					len = destBuf - destBufBegin;											\
					if (rowBytes > 250) {													\
						PICTWriteInt(ptr, len);												\
						byteCount += 2;														\
					} else {																\
						PICTWriteByte(ptr, len);											\
						byteCount++;														\
					}																		\
																							\
					memcpy(ptr, destBufBegin, len);											\
					(char*)(ptr) += len;													\
					byteCount += len;														\
				}																			\
				free(destBufBegin);															\
			}																				\
																							\
			if (byteCount % 2)																\
				PICTWriteFillByte(ptr);														\
		}

/* write text */
#define PICTWriteText(ptr, textptr /* pascal string*/)										\
		{																					\
			memcpy(ptr, textptr, textptr[0]+1);				/* copy string */				\
			(char*)(ptr) += textptr[0]+1;													\
		}



/****************/
/* PICT Opcodes */
/****************/


#define PICT_NOP(ptr)																		\
		{																					\
			PICTWriteOpcode(ptr, 0x0000);						/* NOP opcode */			\
		}

#define PICT_Clip_Rectangular(ptr, x, y, w, h)												\
		{																					\
			PICTWriteOpcode(ptr, 0x0001);						/* Clip opcode */			\
			PICTWriteRegionRectangular(ptr, x, y, w, h);		/* clipRgn */				\
		}

#define PICT_Clip(ptr, x, y, w, h, size, dataptr)											\
		{																					\
			PICTWriteOpcode(ptr, 0x0001);						/* Clip opcode */			\
			PICTWriteRegion(ptr, x, y, w, h, size, dataptr);	/* clipRgn */				\
		}

#define PICT_BkPat(ptr, byte1, byte2, byte3, byte4, byte5, byte6, byte7, byte8)				\
		{																					\
			PICTWriteOpcode(ptr, 0x0002);						/* BkPat opcode */			\
			PICTWritePattern(ptr, byte1, byte2, byte3, byte4,	/* Pattern data */			\
								  byte5, byte6, byte7, byte8);								\
		}

#define PICT_TxFont(ptr, font)																\
		{																					\
			PICTWriteOpcode(ptr, 0x0003);						/* TxFont opcode */			\
			PICTWriteInt(ptr, font);							/* Font number   */			\
		}

#define PICT_TxFace(ptr, style)																\
		{																					\
			PICTWriteOpcode(ptr, 0x0004);						/* TxFace opcode */			\
			PICTWriteByte(ptr, style);							/* Font style    */			\
			PICTWriteFillByte(ptr);								/* Fill byte     */			\
		}

#define PICT_TxMode(ptr, mode)																\
		{																					\
			PICTWriteOpcode(ptr, 0x0005);						/* TxMode opcode */			\
			PICTWriteInt(ptr, mode);							/* Source mode   */			\
		}

#define PICT_PnSize(ptr, w, h)																\
		{																					\
			PICTWriteOpcode(ptr, 0x0006);						/* PnSize opcode */			\
			PICTWritePoint(w, h);								/* Pen size      */			\
		}

#define PICT_PnMode(ptr, mode)																\
		{																					\
			PICTWriteOpcode(ptr, 0x0007);						/* PnMode opcode */			\
			PICTWriteInt(ptr, mode);							/* Pen mode      */			\
		}

#define PICT_PnPat(ptr, byte1, byte2, byte3, byte4, byte5, byte6, byte7, byte8)				\
		{																					\
			PICTWriteOpcode(ptr, 0x0009);						/* PnPat opcode */			\
			PICTWritePattern(ptr, byte1, byte2, byte3, byte4,	/* Pattern data */			\
								  byte5, byte6, byte7, byte8);								\
		}

#define PICT_FillPat(ptr, byte1, byte2, byte3, byte4, byte5, byte6, byte7, byte8)			\
		{																					\
			PICTWriteOpcode(ptr, 0x000A);						/* FillPat opcode */		\
			PICTWritePattern(ptr, byte1, byte2, byte3, byte4,	/* Pattern data   */		\
								  byte5, byte6, byte7, byte8);								\
		}

#define PICT_OvSize(ptr, w, h)																\
		{																					\
			PICTWriteOpcode(ptr, 0x000B);						/* OvSize opcode */			\
			PICTWritePoint(w, h);								/* Oval size     */			\
		}

#define PICT_Origin(ptr, dh, dv)															\
		{																					\
			PICTWriteOpcode(ptr, 0x000C);						/* Origin opcode */			\
			PICTWriteInt(ptr, dh);								/* dh            */			\
			PICTWriteInt(ptr, dv);								/* dv            */			\
		}

#define PICT_TxSize(ptr, size)																\
		{																					\
			PICTWriteOpcode(ptr, 0x000D);						/* TxSize opcode */			\
			PICTWriteInt(ptr, size);							/* Text Size     */			\
		}

#define PICT_FgColor(ptr, color)															\
		{																					\
			PICTWriteOpcode(ptr, 0x000E);						/* FgColor opcode   */		\
			PICTWriteLong(ptr, color);							/* Foreground color */		\
		}

#define PICT_BkColor(ptr, color)															\
		{																					\
			PICTWriteOpcode(ptr, 0x000F);						/* BkColor opcode   */		\
			PICTWriteLong(ptr, color);							/* Background color */		\
		}

#define PICT_TxRatio(ptr, num, denom)														\
		{																					\
			PICTWriteOpcode(ptr, 0x0010);						/* TxRatio opcode      */	\
			PICTWritePoint(ptr, num);							/* Numerator (Point)   */	\
			PICTWritePoint(ptr, denom);							/* Denominator (Point) */	\
		}

#define PICT_VersionOp(ptr, version)														\
		{																					\
			PICTWriteOpcode(ptr, 0x0011);						/* VersionOp opcode */		\
			PICTWriteByte(ptr, version);						/* Version          */		\
			PICTWriteFillByte(ptr);								/* Fill byte        */		\
		}

#define PICT_RGBFgCol(ptr, r, g, b)															\
		{																					\
			PICTWriteOpcode(ptr, 0x001A);						/* RGBFgCol opcode  */		\
			PICTWriteRGBColor(ptr, r, g, b);					/* Foreground color */		\
		}

#define PICT_RGBBkCol(ptr, r, g, b)															\
		{																					\
			PICTWriteOpcode(ptr, 0x001B);						/* RGBBkCol opcode  */		\
			PICTWriteRGBColor(ptr, r, g, b);					/* Background color */		\
		}

#define PICT_HiliteMode(ptr)																\
		{																					\
			PICTWriteOpcode(ptr, 0x001C);						/* HiliteMode opcode */		\
		}

#define PICT_HiliteColor(ptr, r, g, b)														\
		{																					\
			PICTWriteOpcode(ptr, 0x001D);						/* HiliteColor opcode */	\
			PICTWriteRGBColor(ptr, r, g, b);					/* Highlight color    */	\
		}

#define PICT_DefHilite(ptr)																	\
		{																					\
			PICTWriteOpcode(ptr, 0x001E);						/* DefHilite opcode */		\
		}

#define PICT_OpColor(ptr, r, g, b)															\
		{																					\
			PICTWriteOpcode(ptr, 0x001F);						/* OpColor opcode */		\
			PICTWriteRGBColor(ptr, r, g, b);					/* Opcolor        */		\
		}

#define PICT_Line(ptr, x0, y0, x1, y1)														\
		{																					\
			PICTWriteOpcode(ptr, 0x0020);						/* Line opcode */			\
			PICTWritePoint(ptr, x0, y0);						/* pnLoc       */			\
			PICTWritePoint(ptr, x1, y1);						/* newPt       */			\
		}

#define PICT_LineFrom(ptr, x, y)															\
		{																					\
			PICTWriteOpcode(ptr, 0x0021);						/* LineFrom opcode */		\
			PICTWritePoint(ptr, x, y);							/* newPt           */		\
		}

#define PICT_ShortLine(ptr, x, y, dh, dv)													\
		{																					\
			PICTWriteOpcode(ptr, 0x0022);						/* ShortLine opcode */		\
			PICTWritePoint(ptr, x, y);							/* pnLoc            */		\
			PICTWriteByte(ptr, dh);								/* dh               */		\
			PICTWriteByte(ptr, dv);								/* dv               */		\
		}

#define PICT_ShortLineFrom(ptr, dh, dv)														\
		{																					\
			PICTWriteOpcode(ptr, 0x0023);						/* ShortLineFrom opcode */	\
			PICTWriteByte(ptr, dh);								/* dh                   */	\
			PICTWriteByte(ptr, dv);								/* dv                   */	\
		}

#define PICT_LongText(ptr, x, y, textptr /* pascal string */)								\
		{																					\
			PICTWriteOpcode(ptr, 0x0028);						/* LongText opcode */		\
			PICTWritePoint(ptr, x, y);							/* Point           */		\
			PICTWriteText(ptr, textptr);						/* text            */		\
			if ((textptr[0]+1) % 2) PICTWriteFillByte(ptr);									\
		}

#define PICT_DHText(ptr, dh, textptr /* pascal string */)									\
		{																					\
			PICTWriteOpcode(ptr, 0x0029);						/* DHText opcode */			\
			PICTWriteByte(ptr, dh);								/* dh            */			\
			PICTWriteText(ptr, textptr);						/* text            */		\
			if (textptr[0] % 2) PICTWriteFillByte(ptr);										\
		}

#define PICT_DVText(ptr, dv, textptr /* pascal string */)									\
		{																					\
			PICTWriteOpcode(ptr, 0x002A);						/* DVText opcode */			\
			PICTWriteByte(ptr, dv);								/* dv            */			\
			PICTWriteText(ptr, textptr);						/* text            */		\
			if (textptr[0] % 2) PICTWriteFillByte(ptr);										\
		}

#define PICT_DHDVText(ptr, dh, dv, textptr /* pascal string */)								\
		{																					\
			PICTWriteOpcode(ptr, 0x002B);						/* DHDVText opcode */		\
			PICTWriteByte(ptr, dh);								/* dh              */		\
			PICTWriteByte(ptr, dv);								/* dv              */		\
			PICTWriteText(ptr, textptr);						/* text            */		\
			if ((textptr[0]+1) % 2) PICTWriteFillByte(ptr);									\
		}

#define PICT_fontName(ptr, id, nameptr /* pascal string */)									\
		{																					\
			PICTWriteOpcode(ptr, 0x002C);						/* fontName opcode */		\
			PICTWriteInt(ptr, nameptr[0]+1+2);					/* data length     */		\
			PICTWriteInt(ptr, id);								/* font id         */		\
			PICTWriteText(ptr, nameptr);						/* text            */		\
			if ((nameptr[0]+1) % 2) PICTWriteFillByte(ptr);									\
		}

#define PICT_frameRect(ptr, x, y, w, h)														\
		{																					\
			PICTWriteOpcode(ptr, 0x0030);						/* frameRect opcode */		\
			PICTWriteRect(ptr, x, y, w, h);						/* Rectangle        */		\
		}

#define PICT_paintRect(ptr, x, y, w, h)														\
		{																					\
			PICTWriteOpcode(ptr, 0x0031);						/* paintRect opcode */		\
			PICTWriteRect(ptr, x, y, w, h);						/* Rectangle        */		\
		}

#define PICT_eraseRect(ptr, x, y, w, h)														\
		{																					\
			PICTWriteOpcode(ptr, 0x0032);						/* eraseRect opcode */		\
			PICTWriteRect(ptr, x, y, w, h);						/* Rectangle        */		\
		}

#define PICT_invertRect(ptr, x, y, w, h)													\
		{																					\
			PICTWriteOpcode(ptr, 0x0033);						/* invertRect opcode */		\
			PICTWriteRect(ptr, x, y, w, h);						/* Rectangle         */		\
		}

#define PICT_fillRect(ptr, x, y, w, h)														\
		{																					\
			PICTWriteOpcode(ptr, 0x0034);						/* fillRect opcode */		\
			PICTWriteRect(ptr, x, y, w, h);						/* Rectangle       */		\
		}

#define PICT_frameSameRect(ptr)																\
		{																					\
			PICTWriteOpcode(ptr, 0x0038);						/* frameSameRect opcode */	\
		}

#define PICT_paintSameRect(ptr)																\
		{																					\
			PICTWriteOpcode(ptr, 0x0039);						/* paintSameRect opcode */	\
		}

#define PICT_eraseSameRect(ptr)																\
		{																					\
			PICTWriteOpcode(ptr, 0x003A);						/* eraseSameRect opcode */	\
		}

#define PICT_invertSameRect(ptr)															\
		{																					\
			PICTWriteOpcode(ptr, 0x003B);						/* invertSameRect opcode */	\
		}

#define PICT_fillSameRect(ptr)																\
		{																					\
			PICTWriteOpcode(ptr, 0x003C);						/* fillSameRect opcode */	\
		}

#define PICT_frameRRect(ptr, x, y, w, h)													\
		{																					\
			PICTWriteOpcode(ptr, 0x0040);						/* frameRRect opcode */		\
			PICTWriteRect(ptr, x, y, w, h);						/* Rectangle         */		\
		}

#define PICT_paintRRect(ptr, x, y, w, h)													\
		{																					\
			PICTWriteOpcode(ptr, 0x0041);						/* paintRRect opcode */		\
			PICTWriteRect(ptr, x, y, w, h);						/* Rectangle         */		\
		}

#define PICT_eraseRRect(ptr, x, y, w, h)													\
		{																					\
			PICTWriteOpcode(ptr, 0x0042);						/* eraseRRect opcode */		\
			PICTWriteRect(ptr, x, y, w, h);						/* Rectangle         */		\
		}

#define PICT_invertRRect(ptr, x, y, w, h)													\
		{																					\
			PICTWriteOpcode(ptr, 0x0043);						/* invertRRect opcode */	\
			PICTWriteRect(ptr, x, y, w, h);						/* Rectangle          */	\
		}

#define PICT_fillRRect(ptr, x, y, w, h)														\
		{																					\
			PICTWriteOpcode(ptr, 0x0044);						/* fillRRect opcode */		\
			PICTWriteRect(ptr, x, y, w, h);						/* Rectangle        */		\
		}

#define PICT_frameSameRRect(ptr)															\
		{																					\
			PICTWriteOpcode(ptr, 0x0048);						/* frameSameRRect opcode */	\
		}

#define PICT_paintSameRRect(ptr)															\
		{																					\
			PICTWriteOpcode(ptr, 0x0049);						/* paintSameRRect opcode */	\
		}

#define PICT_eraseSameRRect(ptr)															\
		{																					\
			PICTWriteOpcode(ptr, 0x004A);						/* eraseSameRRect opcode */	\
		}

#define PICT_invertSameRRect(ptr)															\
		{																					\
			PICTWriteOpcode(ptr, 0x004B);						/* invertSameRRect opcode */\
		}

#define PICT_fillSameRRect(ptr)																\
		{																					\
			PICTWriteOpcode(ptr, 0x004C);						/* fillSameRRect opcode */	\
		}

#define PICT_frameOval(ptr, x, y, w, h)														\
		{																					\
			PICTWriteOpcode(ptr, 0x0050);						/* frameOval opcode */		\
			PICTWriteRect(ptr, x, y, w, h);						/* Rectangle        */		\
		}

#define PICT_paintOval(ptr, x, y, w, h)														\
		{																					\
			PICTWriteOpcode(ptr, 0x0051);						/* paintOval opcode */		\
			PICTWriteRect(ptr, x, y, w, h);						/* Rectangle        */		\
		}

#define PICT_eraseOval(ptr, x, y, w, h)														\
		{																					\
			PICTWriteOpcode(ptr, 0x0052);						/* eraseOval opcode */		\
			PICTWriteRect(ptr, x, y, w, h);						/* Rectangle        */		\
		}

#define PICT_invertOval(ptr, x, y, w, h)													\
		{																					\
			PICTWriteOpcode(ptr, 0x0053);						/* invertOval opcode */		\
			PICTWriteRect(ptr, x, y, w, h);						/* Rectangle         */		\
		}

#define PICT_fillOval(ptr, x, y, w, h)														\
		{																					\
			PICTWriteOpcode(ptr, 0x0054);						/* fillOval opcode */		\
			PICTWriteRect(ptr, x, y, w, h);						/* Rectangle       */		\
		}

#define PICT_frameSameOval(ptr)																\
		{																					\
			PICTWriteOpcode(ptr, 0x0058);						/* frameSameOval opcode */	\
		}

#define PICT_paintSameOval(ptr)																\
		{																					\
			PICTWriteOpcode(ptr, 0x0059);						/* paintSameOval opcode */	\
		}

#define PICT_eraseSameOval(ptr)																\
		{																					\
			PICTWriteOpcode(ptr, 0x005A);						/* eraseSameOval opcode */	\
		}

#define PICT_invertSameOval(ptr)															\
		{																					\
			PICTWriteOpcode(ptr, 0x005B);						/* invertSameOval opcode */	\
		}

#define PICT_fillSameOval(ptr)																\
		{																					\
			PICTWriteOpcode(ptr, 0x005C);						/* fillSameOval opcode */	\
		}

#define PICT_frameArc(ptr, x, y, w, h, startAngle, arcAngle)								\
		{																					\
			PICTWriteOpcode(ptr, 0x0060);						/* frameArc opcode */		\
			PICTWriteRect(ptr, x, y, w, h);						/* Rectangle       */		\
			PICTWriteInt(ptr, startAngle);						/* startAngle      */		\
			PICTWriteInt(ptr, arcAngle);						/* arcAngle        */		\
		}

#define PICT_paintArc(ptr, x, y, w, h, startAngle, arcAngle)								\
		{																					\
			PICTWriteOpcode(ptr, 0x0061);						/* paintArc opcode */		\
			PICTWriteRect(ptr, x, y, w, h);						/* Rectangle       */		\
			PICTWriteInt(ptr, startAngle);						/* startAngle      */		\
			PICTWriteInt(ptr, arcAngle);						/* arcAngle        */		\
		}

#define PICT_eraseArc(ptr, x, y, w, h, startAngle, arcAngle)								\
		{																					\
			PICTWriteOpcode(ptr, 0x0062);						/* eraseArc opcode */		\
			PICTWriteRect(ptr, x, y, w, h);						/* Rectangle       */		\
			PICTWriteInt(ptr, startAngle);						/* startAngle      */		\
			PICTWriteInt(ptr, arcAngle);						/* arcAngle        */		\
		}

#define PICT_invertArc(ptr, x, y, w, h, startAngle, arcAngle)								\
		{																					\
			PICTWriteOpcode(ptr, 0x0063);						/* invertArc opcode */		\
			PICTWriteRect(ptr, x, y, w, h);						/* Rectangle        */		\
			PICTWriteInt(ptr, startAngle);						/* startAngle       */		\
			PICTWriteInt(ptr, arcAngle);						/* arcAngle         */		\
		}

#define PICT_fillArc(ptr, x, y, w, h, startAngle, arcAngle)									\
		{																					\
			PICTWriteOpcode(ptr, 0x0064);						/* fillArc opcode */		\
			PICTWriteRect(ptr, x, y, w, h);						/* Rectangle      */		\
			PICTWriteInt(ptr, startAngle);						/* startAngle     */		\
			PICTWriteInt(ptr, arcAngle);						/* arcAngle       */		\
		}

/* use only with rowBytes < 8 !! */
#define PICT_BitsRect_BitMap(ptr, x0, y0, w0, h0, x1, y1, w1, h1, rowBytes, mode, dataPtr)	\
		{																					\
			PICTWriteOpcode(ptr, 0x0090);						/* BitsRect opcode */		\
			PICTWriteInt(ptr, rowBytes);						/* rowBytes        */		\
			PICTWriteRect(ptr, x1, y1, w1, h1);					/* bounds x1????   */		\
			PICTWriteRect(ptr, x0, y0, w0, h0);					/* srcRect         */		\
			PICTWriteRect(ptr, x1, y1, w1, h1);					/* dstRect         */		\
			PICTWriteInt(ptr, mode);							/* mode            */		\
			memcpy(ptr, dataPtr, h0*rowBytes);					/* BitMap data     */		\
		}

#define PICT_PackBitsRect_BitMap(ptr, x0, y0, w0, h0, x1, y1, w1, h1, rowBytes, mode,		\
								 dataPtr, size)												\
		{																					\
			PICTWriteOpcode(ptr, 0x0098);						/* PackBitsRect opcode */	\
			PICTWriteInt(ptr, rowBytes);						/* rowBytes        */		\
			PICTWriteRect(ptr, x1, y1, w1, h1);					/* bounds x1????   */		\
			PICTWriteRect(ptr, x0, y0, w0, h0);					/* srcRect         */		\
			PICTWriteRect(ptr, x1, y1, w1, h1);					/* dstRect         */		\
			PICTWriteInt(ptr, mode);							/* mode            */		\
			memcpy(ptr, dataPtr, size);							/* BitMap data     */		\
		}

#define PICT_OpEndPic(ptr)																	\
		{																					\
			PICTWriteOpcode(ptr, 0x00FF);						/* OpEndPic opcode */		\
		}

/* same as PICT_OpEndPic, but doesn't move pointer */
#define PICT_OpEndPicGoOn(ptr)																\
		{																					\
			*(ptr) = 0x00FF;									/* OpEndPic opcode */		\
		}








/******************************/
/* ghostscript to PICT macros */
/******************************/

/* set forground color to black and background color to white */
#define GSSetStdCol(ptr)																	\
		{																					\
			PICT_RGBFgCol(ptr, 0x0000, 0x0000, 0x0000);		/* black */						\
			PICT_RGBBkCol(ptr, 0xFFFF, 0xFFFF, 0xFFFF);		/* white */						\
		}

#define GSSetFgCol(dev, ptr, col)															\
		{																					\
			gx_color_value	rgb[3];																	\
			(*dev_proc(dev, map_color_rgb))(dev, col, rgb);									\
			PICT_RGBFgCol(ptr, rgb[0], rgb[1], rgb[2]);										\
		}

#define GSSetBkCol(dev, ptr, col)															\
		{																					\
			gx_color_value	rgb[3];																	\
			(*dev_proc(dev, map_color_rgb))(dev, col, rgb);									\
			PICT_RGBBkCol(ptr, rgb[0], rgb[1], rgb[2]);										\
		}

#endif /* gdevmacpictop_INCLUDED */
