/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
 *	MPEG layer 3 tables include file
 *
 *	Copyright (c) 1999 Albert L Faber
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef LAME_TABLES_H
#define LAME_TABLES_H

#include "machine.h"

typedef struct {
    unsigned char  no;
    unsigned char  width;
    unsigned char  minval_2;
    float          quiet_thr;
    float          norm;
    float          bark;
} type1_t;

typedef struct {
    unsigned char  no;
    unsigned char  width;
    float          quiet_thr;
    float          norm;
    float          SNR;
    float          bark;
} type2_t;

typedef struct {
    unsigned int  no     :  5;
    unsigned int  cbw    :  3;
    unsigned int  bu     :  6;
    unsigned int  bo     :  6;
    unsigned int  w1_576 : 10;
    unsigned int  w2_576 : 10;
} type34_t;

typedef struct {
    size_t                 len1;
    const type1_t*  const  tab1;
    size_t                 len2;
    const type2_t*  const  tab2;
    size_t                 len3;
    const type34_t* const  tab3;
    size_t                 len4;
    const type34_t* const  tab4;
} type5_t;

extern const type5_t  table5 [6];



#define HTN	34

struct huffcodetab {
    const int    xlen; 	        /* max. x-index+			*/
    const int    linmax;	/* max number to be stored in linbits	*/
    const short*   table;	        /* pointer to array[xlen][ylen]		*/
    const char*  hlen;	        /* pointer to array[xlen][ylen]		*/
};

extern const struct huffcodetab ht [HTN];
    /* global memory block			*/
    /* array of all huffcodtable headers	*/
    /* 0..31 Huffman code table 0..31		*/
    /* 32,33 count1-tables			*/

extern const char t32l [];
extern const char t33l [];

extern const unsigned int   largetbl    [16*16];
extern const unsigned int   table23       [3*3];
extern const unsigned int   table56       [4*4];
#endif /* LAME_TABLES_H */
