/* nvidia registers and contents */

/* most of this is unused rubbish */

#define SURFACE_FORMAT		0x300
#define SURFACE_FORMAT_DEPTH8	1
// #define SURFACE_FORMAT_DEPTH15	2
#define SURFACE_FORMAT_DEPTH16	4
#define SURFACE_FORMAT_DEPTH24	6

// #define SURFACE_PITCH	0x304
// #define SURFACE_PITCH_SRC	15:0		/* ugh, bitfields */
// #define SURFACE_PITCH_DST	31:16

// #define SURFACE_OFFSET_SRC	0x308
// #define SURFACE_OFFSET_DST	0x30C

#define ROP_SET			0x2300

#define PATTERN_FORMAT		0x4300
#define PATTERN_FORMAT_DEPTH8	3
#define PATTERN_FORMAT_DEPTH16	1
#define PATTERN_FORMAT_DEPTH24	3

#define PATTERN_COLOR_0		0x4310
// #define PATTERN_COLOR_1	0x4314
// #define PATTERN_PATTERN_0	0x4318
// #define PATTERN_PATTERN_1	0x431C

// #define CLIP_POINT		0x6300
// #define CLIP_POINT_X		15:0
// #define CLIP_POINT_Y		31:16

// #define CLIP_SIZE		0x6304
// #define CLIP_SIZE_WIDTH	15:0
// #define CLIP_SIZE_HEIGHT	31:16

#define LINE_FORMAT		0x8300
#define LINE_FORMAT_DEPTH8	3
#define LINE_FORMAT_DEPTH16	1
#define LINE_FORMAT_DEPTH24	3

// #define LINE_COLOR		0x8304
// #define LINE_MAX_LINES	16

// #define LINE_LINES(i)	0x8400+(i)*8
// #define LINE_LINES_POINT0_X	15:0
// #define LINE_LINES_POINT0_Y	31:16
// #define LINE_LINES_POINT1_X	47:32	/* ugh, vlong bitfields */
// #define LINE_LINES_POINT1_Y	63:48

#define BLIT_POINT_SRC		0xA300
// #define BLIT_POINT_SRC_X	15:0
// #define BLIT_POINT_SRC_Y	31:16

// #define BLIT_POINT_DST	0xA304
// #define BLIT_POINT_DST_X	15:0
// #define BLIT_POINT_DST_Y	31:16

// #define BLIT_SIZE		0xA308
// #define BLIT_SIZE_WIDTH	15:0
// #define BLIT_SIZE_HEIGHT	31:16

#define RECT_FORMAT		0xC300
#define RECT_FORMAT_DEPTH8	3
#define RECT_FORMAT_DEPTH16	1
#define RECT_FORMAT_DEPTH24	3

#define RECT_SOLID_COLOR	0xC3FC
// #define RECT_SOLID_RECTS_MAX_RECTS	32

#define RECT_SOLID_RECTS(i)	0xC400+(i)*8
// #define RECT_SOLID_RECTS_Y	15:0
// #define RECT_SOLID_RECTS_X	31:16
// #define RECT_SOLID_RECTS_HEIGHT	47:32
// #define RECT_SOLID_RECTS_WIDTH	63:48

// #define RECT_EXPAND_ONE_COLOR_CLIP		0xC7EC
// #define RECT_EXPAND_ONE_COLOR_CLIP_POINT0_X	15:0
// #define RECT_EXPAND_ONE_COLOR_CLIP_POINT0_Y	31:16
// #define RECT_EXPAND_ONE_COLOR_CLIP_POINT1_X	47:32
// #define RECT_EXPAND_ONE_COLOR_CLIP_POINT1_Y	63:48

// #define RECT_EXPAND_ONE_COLOR_COLOR		0xC7F4

// #define RECT_EXPAND_ONE_COLOR_SIZE		0xC7F8
// #define RECT_EXPAND_ONE_COLOR_SIZE_WIDTH	15:0
// #define RECT_EXPAND_ONE_COLOR_SIZE_HEIGHT	31:16

// #define RECT_EXPAND_ONE_COLOR_POINT		0xC7FC
// #define RECT_EXPAND_ONE_COLOR_POINT_X	15:0
// #define RECT_EXPAND_ONE_COLOR_POINT_Y	31:16
// #define RECT_EXPAND_ONE_COLOR_DATA_MAX_DWORDS	128

// #define RECT_EXPAND_ONE_COLOR_DATA(i)	0xC800+(i)*4

// #define RECT_EXPAND_TWO_COLOR_CLIP		0xCBE4
// #define RECT_EXPAND_TWO_COLOR_CLIP_POINT0_X	15:0
// #define RECT_EXPAND_TWO_COLOR_CLIP_POINT0_Y	31:16
// #define RECT_EXPAND_TWO_COLOR_CLIP_POINT1_X	47:32
// #define RECT_EXPAND_TWO_COLOR_CLIP_POINT1_Y	63:48

// #define RECT_EXPAND_TWO_COLOR_COLOR_0	0xCBEC
// #define RECT_EXPAND_TWO_COLOR_COLOR_1	0xCBF0

// #define RECT_EXPAND_TWO_COLOR_SIZE_IN	0xCBF4
// #define RECT_EXPAND_TWO_COLOR_SIZE_IN_WIDTH	15:0
// #define RECT_EXPAND_TWO_COLOR_SIZE_IN_HEIGHT	31:16

// #define RECT_EXPAND_TWO_COLOR_SIZE_OUT	0xCBF8
// #define RECT_EXPAND_TWO_COLOR_SIZE_OUT_WIDTH	15:0
// #define RECT_EXPAND_TWO_COLOR_SIZE_OUT_HEIGHT	31:16

// #define RECT_EXPAND_TWO_COLOR_POINT		0xCBFC
// #define RECT_EXPAND_TWO_COLOR_POINT_X	15:0
// #define RECT_EXPAND_TWO_COLOR_POINT_Y	31:16
// #define RECT_EXPAND_TWO_COLOR_DATA_MAX_DWORDS	128

// #define RECT_EXPAND_TWO_COLOR_DATA(i)	0xCC00+(i)*4

// #define STRETCH_BLIT_FORMAT		0xE300
// #define STRETCH_BLIT_FORMAT_DEPTH8	4
// #define STRETCH_BLIT_FORMAT_DEPTH16	7
// #define STRETCH_BLIT_FORMAT_DEPTH24	4
// #define STRETCH_BLIT_FORMAT_X8R8G8B8	4
// #define STRETCH_BLIT_FORMAT_YUYV	5
// #define STRETCH_BLIT_FORMAT_UYVY	6

// #define STRETCH_BLIT_CLIP_POINT	0xE308
// #define STRETCH_BLIT_CLIP_POINT_X	15:0
// #define STRETCH_BLIT_CLIP_POINT_Y	31:16

// #define STRETCH_BLIT_CLIP_SIZE	0xE30C
// #define STRETCH_BLIT_CLIP_SIZE_WIDTH	15:0
// #define STRETCH_BLIT_CLIP_SIZE_HEIGHT	31:16

// #define STRETCH_BLIT_DST_POINT	0xE310
// #define STRETCH_BLIT_DST_POINT_X	15:0
// #define STRETCH_BLIT_DST_POINT_Y	31:16

// #define STRETCH_BLIT_DST_SIZE	0xE314
// #define STRETCH_BLIT_DST_SIZE_WIDTH	15:0
// #define STRETCH_BLIT_DST_SIZE_HEIGHT	31:16

// #define STRETCH_BLIT_DU_DX		0xE318
// #define STRETCH_BLIT_DV_DY		0xE31C

// #define STRETCH_BLIT_SRC_SIZE	0xE400
// #define STRETCH_BLIT_SRC_SIZE_WIDTH	15:0
// #define STRETCH_BLIT_SRC_SIZE_HEIGHT	31:16

// #define STRETCH_BLIT_SRC_FORMAT	0xE404
// #define STRETCH_BLIT_SRC_FORMAT_PITCH	15:0
// #define STRETCH_BLIT_SRC_FORMAT_ORIGIN	23:16
// #define STRETCH_BLIT_SRC_FORMAT_ORIGIN_CENTER	1
// #define STRETCH_BLIT_SRC_FORMAT_ORIGIN_CORNER	2
// #define STRETCH_BLIT_SRC_FORMAT_FILTER	31:24
// #define STRETCH_BLIT_SRC_FORMAT_FILTER_POINT_SAMPLE	0
// #define STRETCH_BLIT_SRC_FORMAT_FILTER_BILINEAR	1

// #define STRETCH_BLIT_SRC_OFFSET	0xE408

// #define STRETCH_BLIT_SRC_POINT	0xE40C
// #define STRETCH_BLIT_SRC_POINT_U	15:0
// #define STRETCH_BLIT_SRC_POINT_V	31:16

 /***************************************************************************\
|*       Copyright 2003 NVIDIA, Corporation.  All rights reserved.           *|
|*                                                                           *|
|*     NOTICE TO USER:   The source code  is copyrighted under  U.S. and     *|
|*     international laws.  Users and possessors of this source code are     *|
|*     hereby granted a nonexclusive,  royalty-free copyright license to     *|
|*     use this code in individual and commercial software.                  *|
|*                                                                           *|
|*     Any use of this source code must include,  in the user documenta-     *|
|*     tion and  internal comments to the code,  notices to the end user     *|
|*     as follows:                                                           *|
|*                                                                           *|
|*       Copyright 2003 NVIDIA, Corporation.  All rights reserved.           *|
|*                                                                           *|
|*     NVIDIA, CORPORATION MAKES NO REPRESENTATION ABOUT THE SUITABILITY     *|
|*     OF  THIS SOURCE  CODE  FOR ANY PURPOSE.  IT IS  PROVIDED  "AS IS"     *|
|*     WITHOUT EXPRESS OR IMPLIED WARRANTY OF ANY KIND.  NVIDIA, CORPOR-     *|
|*     ATION DISCLAIMS ALL WARRANTIES  WITH REGARD  TO THIS SOURCE CODE,     *|
|*     INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY, NONINFRINGE-     *|
|*     MENT,  AND FITNESS  FOR A PARTICULAR PURPOSE.   IN NO EVENT SHALL     *|
|*     NVIDIA, CORPORATION  BE LIABLE FOR ANY SPECIAL,  INDIRECT,  INCI-     *|
|*     DENTAL, OR CONSEQUENTIAL DAMAGES,  OR ANY DAMAGES  WHATSOEVER RE-     *|
|*     SULTING FROM LOSS OF USE,  DATA OR PROFITS,  WHETHER IN AN ACTION     *|
|*     OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,  ARISING OUT OF     *|
|*     OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOURCE CODE.     *|
|*                                                                           *|
|*     U.S. Government  End  Users.   This source code  is a "commercial     *|
|*     item,"  as that  term is  defined at  48 C.F.R. 2.101 (OCT 1995),     *|
|*     consisting  of "commercial  computer  software"  and  "commercial     *|
|*     computer  software  documentation,"  as such  terms  are  used in     *|
|*     48 C.F.R. 12.212 (SEPT 1995)  and is provided to the U.S. Govern-     *|
|*     ment only as  a commercial end item.   Consistent with  48 C.F.R.     *|
|*     12.212 and  48 C.F.R. 227.7202-1 through  227.7202-4 (JUNE 1995),     *|
|*     all U.S. Government End Users  acquire the source code  with only     *|
|*     those rights set forth herein.                                        *|
 \***************************************************************************/

/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/nv/nv_dma.h,v 1.2 2003/07/31 21:41:26 mvojkovi Exp $ */
