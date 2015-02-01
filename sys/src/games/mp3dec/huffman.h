/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
 * libmad - MPEG audio decoder library
 * Copyright (C) 2000-2004 Underbit Technologies, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * $Id; //  huffman.h,v 1.11 2004/01/23 09; // 41; // 32 rob Exp $
 */

# ifndef LIBMAD_HUFFMAN_H
# define LIBMAD_HUFFMAN_H

struct huffquad {
  unsigned char final;
  struct {
    unsigned char bits;
    unsigned short offset;
  } ptr;
  struct {
    unsigned char hlen;
    unsigned char v, w, x, y;
  } value;
};

struct huffpair {
  unsigned char final;
  struct {
    unsigned char bits;
    unsigned short offset;
  } ptr;
  struct {
    unsigned char hlen;
    unsigned char x;
    unsigned char y;
  } value;
};

struct hufftable {
  struct huffpair const *table;
  unsigned short linbits;
  unsigned short startbits;
};

extern struct huffquad const *const mad_huff_quad_table[2];
extern struct hufftable const mad_huff_pair_table[32];

# endif
