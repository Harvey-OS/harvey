/*
 * Copyright (c) 1999 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 * 
 * The contents of this file constitute Original Code as defined in and
 * are subject to the Apple Public Source License Version 1.1 (the
 * "License").  You may not use this file except in compliance with the
 * License.  Please obtain a copy of the License at
 * http://www.apple.com/publicsource and read it before using this file.
 * 
 * This Original Code and all software distributed under the License are
 * distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE OR NON-INFRINGEMENT.  Please see the
 * License for the specific language governing rights and limitations
 * under the License.
 * 
 * @APPLE_LICENSE_HEADER_END@
 */
/*
 * Copyright (c) 1990, 1993
 *      The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Mike Hibler and Chris Torek.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by the University of
 *      California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#include <u.h>
#include <libc.h>

#define wsize   sizeof(uint64_t)
#define wmask   (wsize - 1)

void *
memset(void* dest, int c0, size_t length)
{
        size_t t;
        uint64_t c = 0;
        unsigned char *dst;

        dst = dest;
        /*
         * If not enough words, just fill bytes.  A length >= 2 words
         * guarantees that at least one of them is `complete' after
         * any necessary alignment.  For instance:
         *
         *      |-----------|-----------|-----------|
         *      |00|01|02|03|04|05|06|07|08|09|0A|00|
         *                ^---------------------^
         *               dst             dst+length-1
         *
         * but we use a minimum of 3 here since the overhead of the code
         * to do word writes is substantial.
         */ 
        if (length < 3 * wsize) {
                while (length != 0) {
                        *dst++ = c0;
                        --length;
                }
                return dest;
        }

        if ((c = (unsigned char)c0) != 0) {     /* Fill the word. */
               c = (c << 8) | c;       
               c = (c << 16) | c;
               c = (c << 32) | c;      /* uint64_t is 64 bits. */
        }
        /* Align destination by filling in bytes. */
        if ((t = (uintptr_t)dst & wmask) != 0) {
                t = wsize - t;
                length -= t;
                do {
                        *dst++ = c0;
                } while (--t != 0);
        }

        /* Fill words.  Length was >= 2*words so we know t >= 1 here. */
        t = length / wsize;
        do {
                *(uint64_t *)dst = c;
                dst += wsize;
        } while (--t != 0);

        /* Mop up trailing bytes, if any. */
        t = length & wmask;
        if (t != 0)
                do {
                        *dst++ = c0;
                } while (--t != 0);
        return dest;
}
